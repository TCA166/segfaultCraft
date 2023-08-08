#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80
#define MAX_VAR_INT 5
#define MAX_VAR_LONG 10
#define timeout 500000 //ms
#define NO_COMPRESSION -1
#define MAX_PACKET 4096

#define packetNull(packet) packet.data == NULL && packet.size == -1 && packet.packetId == 0

typedef unsigned char byte;

typedef __uint128_t UUID;

typedef struct packet{
    int size;
    byte packetId;
    byte* data;
} packet;

//Connects the given socket to the given host to the given port
int connectSocket(int socketFd, const char* host, int port);

//Writes the given value to the buffer as VarInt
size_t writeVarInt(byte* buff, int value);

//Sends the given packet to the Minecraft server in the correct format
ssize_t sendPacket(int socketFd, int size, int packetId, const byte* data, int compression);

//Writes the given string to the buffer with a VarInt length preceding it
size_t writeString(byte* buff, const char* string, int stringLen);

//Reads the VarInt from buffer at the given index
int readVarInt(const byte* buff, int* index);

//Tries getting the raw packet data as long as it doesn't timeout
byte* getPacket(int socketFd);

//Requests a packet from server in the correct format
ssize_t requestPacket(int socketFd, int packetType, int compression);

//Performs a specific handshake
ssize_t handshake(int socketFd, const char* host, int protocol, short port, int nextState);

//Parses the raw packet data into a nice struct
packet parsePacket(const byte* data, int compression);

//Performs a ping-pong interaction with the server and returns the delay
int64_t pingPong(int socketFd);

//Starts the login process
int startLogin(int socketFd, const char* name, const UUID* player);

//Returns a pointer to a string within buff
char* readString(const byte* buff, int* index);