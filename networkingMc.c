#include "networkingMc.h"
#include "packetDefinitions.h"

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#if defined(__unix__)
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#elif defined(_WIN32)
#include <conio.h>
#include <winsock2.h>
#include <io.h>
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
#endif

#include <zlib.h>

//Private functions

/*!
 @brief Reads the socket
 @param socketFd the socket file descriptor
 @return the byteArray stored within the socket, or a NULL byteArray on error or EOF
*/
byteArray readSocket(int socketFd);

/*!
 @brief Tries getting a byteArray from a socket until error or timeout
 @param socketFd the socket file descriptor
 @return the packet data alongside the length of the data as byteArray, or a NULL byteArray on error or timeout
*/
byteArray getPacketBytes(int socketFd);

/*!
 @brief Parses the raw packet data into a nice struct
 @param dataArray pointer to the packet data alongside it's size
 @param compression the established compression level
 @return the packet struct or a nullPacket to indicate error
*/
packet parsePacket(byteArray* dataArray, int compression);

/*Socket reading note
The process of getting a valid packet goes like this:
readSocket -tries reading-> getPacketBytes -tries reading until timeout-> parsePacket -parses the packet-> getPacket()
*/

ssize_t requestPacket(int socketFd, int packetType, int compression){
    return sendPacket(socketFd, 0, packetType, NULL, compression);
}

ssize_t handshake(int socketFd, const char* host, int protocol, short port, int nextState){
    size_t hostLen = strlen(host);
    size_t handshakeLen = sizeof(int16_t) + (MAX_VAR_INT * 3) + hostLen;
    byte* handshake = calloc(handshakeLen, sizeof(byte));
    size_t off1 = writeVarInt(handshake, protocol); //max 5 bytes
    size_t off2 = writeString(handshake + off1, host, hostLen);
    memcpy(handshake + off1 + off2, &port, sizeof(int16_t));
    size_t off3 = writeVarInt(handshake + off1 + off2 + sizeof(int16_t), nextState);
    handshakeLen = sizeof(int16_t) + off1 + off2 + off3;
    ssize_t res = sendPacket(socketFd, handshakeLen, HANDSHAKE, handshake, NO_COMPRESSION);
    free(handshake);
    return res;
}

ssize_t sendPacket(int socketFd, int size, int packetId, const byte* data, int compression){
    if(compression <= NO_COMPRESSION){ //packet type without compression
        byte p[MAX_VAR_INT] = {};
        size_t pSize = writeVarInt(p, packetId);
        byte s[MAX_VAR_INT] = {};
        size_t sSize = writeVarInt(s, size + pSize);
        if(write(socketFd, s, sSize) == -1 || write(socketFd, p, pSize) == -1){
            return -1;
        }
        if(data != NULL){
            return write(socketFd, data, size);
        }
        return sSize + 1;
    }
    else{
        byte* dataToCompress = calloc(MAX_VAR_INT + size, sizeof(byte));
        size_t offset = writeVarInt(dataToCompress, packetId);
        memcpy(dataToCompress + offset, data, size);
        //ok so now we can compress
        int dataLength = offset + size;
        uLongf destLen = dataLength;
        if(size + offset > compression){
            byte* compressed = calloc(destLen, sizeof(byte));
            if(compress(compressed, &destLen, dataToCompress, destLen) != Z_OK){
                return -1;
            }
            free(dataToCompress);
            dataToCompress = compressed;
        }
        else{
            dataLength = 0;
        }
        byte* packet = calloc(destLen + (MAX_VAR_INT * 2), sizeof(byte));
        byte l[MAX_VAR_INT] = {};
        size_t sizeL = writeVarInt(l, dataLength);
        size_t sizeP = writeVarInt(packet, destLen + sizeL);
        memcpy(packet + sizeP, l, sizeL);
        memcpy(packet + sizeP + sizeL, dataToCompress, destLen);
        return write(socketFd, packet, destLen + sizeP + sizeL);
    }
}

byteArray getPacketBytes(int socketFd){
    time_t start = clock();
    byteArray packet = {NULL, 0};
    while(packet.bytes == NULL){
        packet = readSocket(socketFd);
        time_t now = clock();
        if(now - start > TCP_TIMEOUT){
            errno = ETIMEDOUT;
            return packet;
        }
    }
    return packet;
}

byteArray readSocket(int socketFd){
    byteArray result = {NULL, 0};
    int size = 0;
    int position = 0;
    while(true){
        byte curByte = 0;
        int res = read(socketFd, &curByte, 1);
        if(res < 1){
            return result;
        }
        size |= (curByte & SEGMENT_BITS) << position;
        if((curByte & CONTINUE_BIT) == 0) break;
        position += 7;
    }
    result.len = size;
    byte* input = calloc(size, sizeof(byte));
    int nRead = 0;
    //While we have't read the entire packet
    while(nRead < size){
        //Read the rest of the packet
        int r = read(socketFd, input + nRead, size - nRead);
        if(r < 1){
            free(input);
            return result;
        }
        nRead += r;
    }
    result.bytes = input;
    return result;
}

packet parsePacket(byteArray* dataArray, int compression){
    packet result = nullPacket;
    int index = 0;
    bool alloc = false;
    byte* data = dataArray->bytes;
    if(compression > -1){
        uLongf dataLength = (uLongf)readVarInt(data, &index);
        int compressedLen = dataArray->len - index;
        if(dataLength >= compression){
            byte* compressed = (byte*)data + index;
            byte* uncompressed = NULL;
            uncompressed = calloc(dataLength, sizeof(byte));
            alloc = true;
            if(uncompress(uncompressed, &dataLength, compressed, compressedLen) != Z_OK){
                free(uncompressed);
                return nullPacket;
            } 
            data = uncompressed;
            index = 0;
            result.size = dataLength;
        }
        else{
            result.size = compressedLen;
        }
    }
    else{
        result.size = dataArray->len;
    }
    result.packetId = readVarInt(data, &index);
    result.data = calloc(result.size - 1, sizeof(byte));
    memcpy(result.data, data + index, result.size - 1);
    if(alloc){ //we discard the const since actually we no longer have a const value
        free((byte*)data);
    }
    return result;
}

int64_t pingPong(int socketFd){
    //ping
    int64_t now = (int64_t)clock();
    byte timeBuff[sizeof(int64_t)] = {};
    memcpy(timeBuff, &now, sizeof(int64_t));
    if(sendPacket(socketFd, sizeof(int64_t), PING_REQUEST, timeBuff, NO_COMPRESSION) < 1){
        return -1;
    }
    //pong
    byteArray input = getPacketBytes(socketFd);
    if(input.bytes == NULL){
        return -2;
    }
    packet pong = parsePacket(&input, NO_COMPRESSION);
    free(input.bytes);
    if(pong.data == NULL || pong.packetId != PING_RESPONSE){
        return -3;
    }
    int64_t diff = *((int64_t*)pong.data) - now;
    free(pong.data);
    return diff;
}

int connectSocket(int socketFd, const char* host, int port){
    struct sockaddr_in serverAddress;
    //we initialize the variable
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(host);
    serverAddress.sin_port = htons(port);
    //then we connect the socket
    return connect(socketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
}

ssize_t startLogin(int socketFd, const char* name, const UUID_t* player){
    byte data[16 + MAX_VAR_INT + 1 + sizeof(UUID_t)] = {};
    size_t offset = writeString(data, name, strlen(name));
    size_t packetLen = offset + 1;
    if(player == NULL){
        data[offset] = false;
        bzero(data + offset, sizeof(UUID_t));
    }
    else{
        data[offset] = true;
        memcpy(data + offset, player, sizeof(UUID_t));
        packetLen += sizeof(UUID_t);
    }
    return sendPacket(socketFd, packetLen, LOGIN_START, data, NO_COMPRESSION);
}

packet getPacket(int socketFd, int compression){
    byteArray newPacket = getPacketBytes(socketFd);;
    if(newPacket.bytes == NULL){
        return nullPacket;
    }
    packet response = parsePacket(&newPacket, compression);
    free(newPacket.bytes);
    return response;
}

char* getServerStatus(int socketFd){
    requestPacket(socketFd, STATUS_REQUEST, NO_COMPRESSION);
    //parse the status
    packet status = getPacket(socketFd, NO_COMPRESSION);
    if(status.packetId != STATUS_RESPONSE){
        return NULL;
    }
    int offset = 0;
    int length = readVarInt(status.data, &offset);
    char* rawJson = calloc(length, 1);
    memcpy(rawJson, status.data + offset, length);
    free(status.data);
    //Might use cJson to parse this
    return rawJson;
}
