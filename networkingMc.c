#include "networkingMc.h"
#include "packetDefinitions.h"
#include "cJSON/cJSON.h"

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
#include <bcrypt.h> 
#define getentropy(ptr, len) BCryptGenRandom(NULL, ptr, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG)  
#endif

#include <zlib.h>

//Private functions

/*!
 @brief Reads the socket
 @param socketFd the socket file descriptor
 @return the byteArray stored within the socket, or a NULL byteArray on error or EOF
*/
static byteArray readSocket(int socketFd);

/*!
 @brief Parses the raw packet data into a nice struct
 @param dataArray pointer to the packet data alongside it's size
 @param compression the established compression level
 @return the packet struct or a nullPacket to indicate error
*/
static packet parsePacket(const byteArray* dataArray, int compression);

/*Socket reading note
The process of getting a valid packet goes like this:
readSocket -tries reading-> getPacketBytes -tries reading until timeout-> parsePacket -parses the packet-> getPacket()
*/

ssize_t requestPacket(int socketFd, int packetType, int compression){
    return sendPacket(socketFd, 0, packetType, NULL, compression);
}

ssize_t handshake(int socketFd, const char* host, int32_t protocol, uint16_t port, int32_t nextState){
    size_t hostLen = strlen(host);
    size_t handshakeLen = sizeof(int16_t) + (MAX_VAR_INT * 3) + hostLen;
    byte* handshake = calloc(handshakeLen, sizeof(byte));
    size_t off1 = writeVarInt(handshake, protocol); //max 5 bytes
    size_t off2 = writeString(handshake + off1, host, hostLen);
    size_t off3 = writeBigEndianUShort(handshake + off1 + off2, port);
    size_t off4 = writeVarInt(handshake + off1 + off2 + off3, nextState);
    handshakeLen = off1 + off2 + off3 + off4;
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
        free(dataToCompress);
        ssize_t res = write(socketFd, packet, destLen + sizeP + sizeL);
        free(packet);
        return res;
    }
}

static byteArray readSocket(int socketFd){
    byteArray result = nullByteArray;
    int size = 0;
    int position = 0;
    while(true){
        byte curByte = 0;
        if(read(socketFd, &curByte, 1) < 1){
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

static packet parsePacket(const byteArray* dataArray, int compression){
    packet result = nullPacket;
    int index = 0;
    bool alloc = false;
    byte* data = dataArray->bytes;
    if(compression > NO_COMPRESSION){
        uLongf dataLength = (uLongf)readVarInt(data, &index);
        int compressedLen = dataArray->len - index;
        if(dataLength != 0){
            byte* uncompressed = calloc(dataLength, sizeof(byte));
            alloc = true;
            if(uncompress(uncompressed, &dataLength, data + index, compressedLen) != Z_OK){
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
    result.size -= 1; //packetId will should take up a byte at most
    result.data = calloc(result.size, sizeof(byte));
    memcpy(result.data, data + index, result.size);
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
    packet pong = getPacket(socketFd, NO_COMPRESSION);
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
    byteArray newPacket = readSocket(socketFd);
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

int loginState(int socketFd, packet* response, UUID_t* given, const char* username, int* compression){
    bool login = true;
    while(login){ //loop for handling the login sequence
        int offset = 0;
        switch(response->packetId){
            case DISCONNECT_LOGIN:; //Disconnected
                printf("Disconnected:%s\n", readString(response->data, NULL));
                return 1;
                break;
            case ENCRYPTION_REQUEST:; //We are dealing with an online only server... shit
                //read the data from the request
                char* serverId = readString(response->data, &offset);
                byteArray publicKey = readByteArray(response->data, &offset);
                byteArray verifyToken = readByteArray(response->data, &offset);
                //generate the secret
                byte secret[16] = {};
                if(getentropy(secret, 16) < 0){
                    return -1;
                }
                //now we need to encrypt using AES/CFB8 stream cipher
                //TODO implement
                free(serverId);
                free(publicKey.bytes);
                free(verifyToken.bytes);
                break;
            case LOGIN_SUCCESS:; //Login successful
                *given = *(UUID_t*)response->data;
                offset += sizeof(UUID_t);
                int userNameLen = readVarInt(response->data, &offset);
                if(memcmp(username, (char*)response->data + offset, userNameLen) != 0){
                    errno = EINVAL;
                    return -2;
                }
                offset += userNameLen;
                //int numProperties = readVarInt(response->data, &offset);
                login = false;
                break;
            case SET_COMPRESSION:; //Set compression
                int compressionInput = readVarInt(response->data, NULL);
                if(compressionInput > NO_COMPRESSION){
                    *compression = compressionInput;
                }
                break;
            case LOGIN_PLUGIN_REQUEST:;
                //Just like the notchian client we send an answer that we didn't understand
                int messageId = readVarInt(response->data, &offset);
                //char* channel = readString(response.data, &offset);
                //free(channel);
                byte packet[MAX_VAR_INT + 1] = {};
                int off = writeVarInt(packet, messageId);
                packet[off] = false;
                sendPacket(socketFd, off + 1, LOGIN_PLUGIN_RESPONSE, packet, *compression);
                break;
            default:;
                errno = EPROTO;
                return -3;
        }
        free(response->data);
        *response = getPacket(socketFd, *compression);
        if(packetNull((*response))){
            return -4;
        }
    }
    return 0;
}

//Shouldn't leak memory in case of error
int playState(struct gamestate* current, packet response, int socketFd, int compression, const struct gameVersion* thisVersion){
    int result = 1; //array indicating current status
    int index = 0; //current index within backlog
    packet* backlog = NULL;
    while(true){ //as long as result == 1
        //there are some packet types that need immediate answer and separate processing
        switch (response.packetId){
            case BUNDLE_DELIMITER:{
                if(backlog == NULL){
                    backlog = calloc(MAX_PACKET, sizeof(packet));
                }
                else{
                    //process the backlog
                    for(int i = 0; i < index; i++){
                        if(parsePlayPacket(backlog + i, current, thisVersion) != 0){
                            result = -1;
                            //free all the following packets
                            for(int n = i + 1; n < index; n++){
                                free((backlog + n)->data);
                            }
                            free((backlog + i)->data);
                            break;
                        }
                        free((backlog + i)->data);
                    }
                    free(backlog);
                    backlog = NULL;
                    index = 0;
                }
                free(response.data);
                break;
            }
            case DISCONNECT_PLAY:{
                printf("Disconnected:%s\n", readString(response.data, NULL));
                result = 0;
                free(response.data);
                break;
            }
            case KEEP_ALIVE:{
                int64_t aliveId = *(int64_t*)response.data;
                sendPacket(socketFd, sizeof(int64_t), KEEP_ALIVE_2, (byte*)&aliveId, compression);
                free(response.data);
                break;
            }
            case PING_PLAY:{
                //ping (the vanilla client doesn't respond)
                int32_t pingId = *(int32_t*)response.data;
                sendPacket(socketFd, sizeof(int32_t), PONG_PLAY, (byte*)&pingId, compression);
                free(response.data);
                break;
            }
            case SYNCHRONIZE_PLAYER_POSITION:{
                //this is the responsive version of the handler
                int offset = 0;
                handleSynchronizePlayerPosition(&response, current, &offset);
                int32_t teleportId = readVarInt(response.data, &offset);
                byte packet[MAX_VAR_INT] = {};
                size_t sz = writeVarInt(packet, teleportId);
                sendPacket(socketFd, sz, CONFIRM_TELEPORTATION, packet, compression);
                free(response.data);
                break;
            }
            default:{
                if(backlog == NULL){
                    //process the packet
                    if(parsePlayPacket(&response, current, thisVersion) != 0){
                        result = -2;
                    }
                    free(response.data);
                }
                else{
                    backlog[index] = response;
                    index++;
                }
                break;
            }
        }
        if(result != 1){
            break;
        }
        response = getPacket(socketFd, compression);
        if(packetNull(response)){
            perror("Error while getting a packet");
            return -3;
        }
    };
    return result;
}
