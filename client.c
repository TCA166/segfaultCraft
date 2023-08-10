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
#include <arpa/inet.h>
#include <netdb.h>
#elif defined(_WIN32)
#include <conio.h>
#include <winsock2.h>
#include <io.h>
#endif

#include "networkingMc.h"
#include "packetDefinitions.h"

//https://wiki.vg/Protocol#Definitions

int main(int argc, char** argv){
    if(argc < 4){
        fprintf(stderr, "Invalid number of arguments.");
        return -1;
    }
    char* host = argv[1];
    int16_t port = (int16_t)atoi(argv[2]);
    short protocol = (short)atoi(argv[3]);
    //Our internet socket
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockFd < 0){
        perror("Socket creation failed");
        return -1;
    }
    if(connectSocket(sockFd, host, port) != 0){
        perror("Connection to server failed");
        return -1;
    }
    printf("Connected to the server\n");
    fflush(stdout);
    //now we are ready to communicate
    //let's start by doing the handshake
    handshake(sockFd, host, protocol, port, STATUS_STATE);
    //request the status
    char* status = getServerStatus(sockFd);
    if(status == NULL){
        fprintf(stderr, "Invalid packet received instead of the server status\n");
        return -1;
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
        return -1;
    }
    connectSocket(sockFd, host, port);
    handshake(sockFd, host, protocol, port, LOGIN_STATE);
    const char* username = "Botty";
    startLogin(sockFd, username, NULL);
    packet response = getPacket(sockFd, NO_COMPRESSION);
    { //parse login response

    }
    UUID_t given = 0;
    int compression = NO_COMPRESSION;
    {//Login state
        bool login = true;
        while(login){ //loop for handling the login sequence
            int offset = 0;
            switch(response.packetId){
                case DISCONNECT_LOGIN:; //Disconnected
                    printf("Disconnected:%s\n", readString(response.data, NULL));
                    return -1;
                    break;
                case ENCRYPTION_REQUEST:; //Encryption request
                    
                    break;
                case LOGIN_SUCCESS:; //Login successful
                    given = *(UUID_t*)response.data;
                    int userNameLen = readVarInt(response.data + sizeof(UUID_t), &offset);
                    if(memcmp(username, (char*)response.data + sizeof(UUID_t) + offset, userNameLen) != 0){
                        fprintf(stderr, "Server returned different username to the one given\n");
                        return -1;
                    }
                    login = false;
                    break;
                case SET_COMPRESSION:; //Set compression
                    int compressionInput = readVarInt(response.data, NULL);
                    if(compressionInput > NO_COMPRESSION){
                        compression = compressionInput;
                    }
                    break;
                default:;
                    fprintf(stderr, "Unexpected packet:%d", response.packetId);
                    return -1;
            }
            free(response.data);
            response = getPacket(sockFd, compression);
            if(packetNull(response)){
                perror("Error while getting a packet");
                return -1;
            }
        }
    }
    printf("Successfully logged in\n");
    { //Play state
        bool loginPlay = false; //We should wait for login(play) before proceeding
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
                        free(backlog);
                        backlog = NULL;
                        index = 0;
                    }
                    break;
                case DISCONNECT_PLAY:; //disconnect
                    printf("Disconnected:%s\n", readString(response.data, NULL));
                    return -1;
                    break;
                case KEEP_ALIVE:; //Keep alive
                    int64_t aliveId = *(int64_t*)response.data;
                    sendPacket(sockFd, sizeof(int64_t), KEEP_ALIVE_2, (byte*)&aliveId, compression);
                    break;
                case LOGIN_PLAY:; //login (play)
                    loginPlay = true;
                    break;
                case PING_PLAY:; //ping (the vanilla client doesn't respond)
                    int32_t pingId = *(int32_t*)response.data;
                    sendPacket(sockFd, sizeof(int32_t), PONG_PLAY, (byte*)&pingId, compression);
                    break;
                default:;
                    if(backlog == NULL){
                        //process the packet
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
                return -1;
            }
        }
    }
    return 0;
}

