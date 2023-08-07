#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include "networkingMc.h"

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
    handshake(sockFd, host, protocol, port, 1);
    //request the status
    requestPacket(sockFd, 0x00, NO_COMPRESSION);
    {
        //parse the status
        byte* input = getPacket(sockFd);
        if(input == NULL){
            perror("Couldn't get the packet");
            return -1;
        }
        packet status = parsePacket(input, NO_COMPRESSION);
        free(input);
        if(status.packetId != 0x00){
            fprintf(stderr, "Invalid packet received %d\n", status.packetId);
            return -1;
        }
        int offset = 0;
        int length = readVarInt(status.data, &offset);
        char* rawJson = calloc(length, 1);
        memcpy(rawJson, status.data + offset, length);
        free(status.data);
        //Might use cJson to parse this
    }
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
    handshake(sockFd, host, protocol, port, 2);
    const char* username = "Botty";
    startLogin(sockFd, username, NULL);
    packet response;
    { //parse login response
        byte* loginResponse = getPacket(sockFd);
        if(loginResponse == NULL){
            perror("Login failed");
            return -1;
        }
        response = parsePacket(loginResponse, NO_COMPRESSION);
        if(packetNull(response)){
            perror("Packet parsing failed");
            return -1;
        }
        free(loginResponse);
    }
    UUID given = 0;
    int compression = NO_COMPRESSION;
    {//Login state
        bool login = true;
        while(login){ //loop for handling the login sequence
            int offset = 0;
            switch(response.packetId){
                case 0x00:; //Disconnected
                    printf("Disconnected:%s\n", readString(response.data, NULL));
                    return -1;
                    break;
                case 0x01:; //Encryption request
                    
                    break;
                case 0x02:; //Login successful
                    given = *(UUID*)response.data;
                    int userNameLen = readVarInt(response.data + sizeof(UUID), &offset);
                    if(memcmp(username, (char*)response.data + sizeof(UUID) + offset, userNameLen) != 0){
                        fprintf(stderr, "Server returned different username to the one given\n");
                        return -1;
                    }
                    login = false;
                    break;
                case 0x03:; //Set compression
                    int compressionInput = readVarInt(response.data, NULL);
                    if(compressionInput > NO_COMPRESSION){
                        compression = compressionInput;
                    }
                    break;
                default:;
                    fprintf(stderr, "Unexpected packet:%d", response.packetId);
                    return -1;
            }
            byte* newPacket = NULL;
            while(newPacket == NULL){
                newPacket = getPacket(sockFd);
            }
            response = parsePacket(newPacket, compression);
            if(packetNull(response)){
                perror("Packet parsing failed");
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
                case 0x00:; //delimiter
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
                case 0x1A:; //disconnect
                    printf("Disconnected:%s\n", readString(response.data, NULL));
                    return -1;
                    break;
                case 0x23:; //Keep alive
                    int64_t aliveId = *(int64_t*)response.data;
                    sendPacket(sockFd, sizeof(int64_t), 0x12, (byte*)&aliveId, compression);
                    break;
                case 0x28:; //login (play)
                    loginPlay = true;
                    break;
                case 0x32:; //ping (the vanilla client doesn't respond)
                    int32_t pingId = *(int32_t*)response.data;
                    sendPacket(sockFd, sizeof(int32_t), 0x20, (byte*)&pingId, compression);
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
            byte* newPacket = NULL;
            while(newPacket == NULL){
                newPacket = getPacket(sockFd);
            }
            response = parsePacket(newPacket, compression);
            if(packetNull(response)){
                perror("Packet parsing failed");
                return -1;
            }
        }
    }
    return 0;
}

