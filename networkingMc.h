#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>

//Constants definitions

#define MAX_VAR_INT 5
#define MAX_VAR_LONG 10
#define TCP_TIMEOUT 500000 //ms
#define NO_COMPRESSION -1
#define MAX_PACKET 4096

//Macros

/*!
 @brief Checks if packet is identical to nullPacket
 @param packet the packet to check
 @return true or false
*/
#define packetNull(packet) packet.data == NULL && packet.size == -1 && packet.packetId == 0
//A packet that is considered to be NULL
#define nullPacket (packet){-1, 0, NULL}

//Types

typedef unsigned char byte;

typedef __uint128_t UUID_t;

/*!
 @struct packet
 @brief A Minecraft style packet
 @param size the size of everything following the VarInt in packet, so sizeof(packetId) + sizeof(data)
 @param packetId the packet id that indicates to the server what the data should contain
 @param data packed values of fields that this packet should contain or NULL 
*/
typedef struct packet{
    int size; //The size of the entire packet, so sizeof(data) + sizeof(packetId)
    byte packetId;
    byte* data;
} packet;

/*!
 @struct byteArray
 @brief An array with an attached size_t
 @param bytes the array itself
 @param len the length of the bytes array
*/
typedef struct byteArray{
    byte* bytes;
    size_t len;
} byteArray;

//Functions

/*! 
 @brief Connects the given socket to the given host to the given port
 @param socketFd file descriptor that identifies a properly configured socket
 @param host host name 
 @param port port under which to establish the connection to the host
 @return -1 for errors and 0 for success
*/
int connectSocket(int socketFd, const char* host, int port);

/*! 
 @brief Writes the given value to the buffer as VarInt
 @param buff pointer to an allocated buffer where the VarInt bytes shall be written to
 @param value the value that will be written to the buffer
 @return the number of bytes written to buff
*/
size_t writeVarInt(byte* buff, int value);

/*! 
 @brief Sends the given packet to the Minecraft server in the correct format
 @param socketFd file descriptor that identifies a properly configured socket
 @param size the size of data
 @param packetId an id that identifies the packet in the Minecraft protocol
 @param data the values of fields of the packet
 @param compression the compression level established, or NO_COMPRESSION
 @return the number of bytes written, EOF or -1 for error
*/
ssize_t sendPacket(int socketFd, int size, int packetId, const byte* data, int compression);

/*! 
 @brief Writes the given string to the buffer with a VarInt length preceding it
 @param buff the buffer that the string will be written to
 @param string the string that is to be written to buff
 @param stringLen the length of string
 @return the number of bytes written, including the size of VarInt in front of the string
*/
size_t writeString(byte* buff, const char* string, int stringLen);

/*! 
 @brief Reads the VarInt from buffer at the given index
 @param buff the buffer from which to read the value
 @param index the index at which the value should be read, is incremented by the number of bytes the VarInt took
 @return the int encoded as VarInt in buff at index
*/
int readVarInt(const byte* buff, int* index);

/*!
 @brief Tries getting the raw packet data as long as it doesn't timeout
 @param socketFd the socket file descriptor
 @return the packet data alongside the length of the data as byteArray
*/
byteArray getPacket(int socketFd);

/*!
 @brief Requests a packet from server in the correct format
 @param socketFd the socket file descriptor
 @param packetType the packetId being requested
 @param compression the established compression level
 @return the number of bytes written, EOF or -1 for error
*/
ssize_t requestPacket(int socketFd, int packetType, int compression);

/*!
 @brief Performs a specific handshake with the server
 @param socketFd the socket file descriptor
 @param host the host name (address) of the server
 @param protocol the protocol version utilized by the client
 @return the number of bytes written, EOF or -1 for error
*/
ssize_t handshake(int socketFd, const char* host, int protocol, short port, int nextState);

/*!
 @brief Parses the raw packet data into a nice struct
 @param dataArray pointer to the packet data alongside it's size
 @param compression the established compression level
 @return the packet struct
*/
packet parsePacket(byteArray* dataArray, int compression);

/*!
 @brief Performs a ping-pong interaction with the server and returns the delay
 @param socketFd the socket file descriptor
 @return the difference between the value sent to the server in Ping and the value received in Pong, or < 0 for error
*/
int64_t pingPong(int socketFd);

/*!
 @brief Starts the login process
 @param socketFd the socket file descriptor
 @param name our client user name
 @param player our client UUID
 @return the number of bytes written, EOF or -1 for error
*/
ssize_t startLogin(int socketFd, const char* name, const UUID_t* player);

/*!
 @brief Parses an encoded string
 @param buff the buffer within which the string is encoded
 @param index the index at which the string is encoded, is incremented by the number of bytes the string and VarInt took
 @return the pointer to the encoded string 
*/
char* readString(const byte* buff, int* index);