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
#endif

#include "networkingMc.h"
#include "packetDefinitions.h"
#include "gamestateMc.h"

#define ENTITIES_JSON "./entities.json"

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
    struct gamestate current = initGamestate();
    result = playState(&current, response, sockFd, compression, ENTITIES_JSON);
    freeList(current.entityList, true);
    freeList(current.chunks, false);
    if(result < 0){
        perror("Error encountered during play state handling");
        return result;
    }
    return EXIT_SUCCESS;
}

