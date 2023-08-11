#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

#if defined(__unix__)
#include <sys/socket.h>
#include <unistd.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <netdb.h>
#elif defined(_WIN32)
#include <conio.h>
#include <winsock2.h>
#include <io.h>
#include <bcrypt.h> 
#define getentropy(ptr, len) BCryptGenRandom(NULL, ptr, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) 
#endif

#include "networkingMc.h"
#include "packetDefinitions.h"
#include "gamestateMc.h"

//https://wiki.vg/Protocol#Definitions

int main(int argc, char** argv){
    if(argc < 4){
        fprintf(stderr, "Invalid number of arguments.");
        return EXIT_FAILURE;
    }
    char* host = argv[1];
    int16_t port = (int16_t)atoi(argv[2]);
    short protocol = (short)atoi(argv[3]);
    //Our internet socket
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockFd < 0){
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }
    if(connectSocket(sockFd, host, port) != 0){
        perror("Connection to server failed");
        return EXIT_FAILURE;
    }
    printf("Connected to the server\n");
    //fflush(stdout);
    //now we are ready to communicate
    //let's start by doing the handshake
    handshake(sockFd, host, protocol, port, STATUS_STATE);
    //request the status
    char* status = getServerStatus(sockFd);
    if(status == NULL){
        fprintf(stderr, "Invalid packet received instead of the server status\n");
        return EXIT_FAILURE;
    }
    free(status);
    //do the ping pong
    int64_t delay = pingPong(sockFd);
    printf("%ld time difference detected.\n", delay);
    close(sockFd); //we have to reset the socket
    //Login state
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockFd < 0){
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }
    connectSocket(sockFd, host, port);
    handshake(sockFd, host, protocol, port, LOGIN_STATE);
    const char* username = "Botty";
    UUID_t player = 0;
    startLogin(sockFd, username, NULL);
    packet response = getPacket(sockFd, NO_COMPRESSION);
    int compression = NO_COMPRESSION;
    //Login state
    int result = loginState(sockFd, &response, &player, username, &compression);
    if(result != 0){
        perror("Error encountered during login");
        return result;
    }
    printf("Successfully logged in\n");
    { //Play state
        struct gamestate current = {};
        bool play = true;
        int index = 0;
        packet* backlog = NULL;
        while(play){
            //there are some packet types that need immediate answer and separate processing
            switch (response.packetId){
                case BUNDLE_DELIMITER:; //delimiter
                    if(backlog == NULL){
                        backlog = calloc(MAX_PACKET, sizeof(packet));
                    }
                    else{
                        //process the backlog
                        for(int i = 0; i < index; i++){
                            parsePlayPacket(backlog + i, &current);
                        }
                        free(backlog);
                        backlog = NULL;
                        index = 0;
                    }
                    break;
                case DISCONNECT_PLAY:; //disconnect
                    printf("Disconnected:%s\n", readString(response.data, NULL));
                    return EXIT_SUCCESS;
                    break;
                case KEEP_ALIVE:; //Keep alive
                    int64_t aliveId = *(int64_t*)response.data;
                    sendPacket(sockFd, sizeof(int64_t), KEEP_ALIVE_2, (byte*)&aliveId, compression);
                    break;
                case PING_PLAY:; //ping (the vanilla client doesn't respond)
                    int32_t pingId = *(int32_t*)response.data;
                    sendPacket(sockFd, sizeof(int32_t), PONG_PLAY, (byte*)&pingId, compression);
                    break;
                default:;
                    if(backlog == NULL){
                        //process the packet
                        parsePlayPacket(&response, &current);
                    }
                    else{
                        backlog[index] = response;
                        index++;
                    }
                    break;
            }
            free(response.data);
            response = getPacket(sockFd, compression);
            if(packetNull(response)){
                perror("Error while getting a packet");
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}

