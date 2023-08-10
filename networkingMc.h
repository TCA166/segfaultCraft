#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>
#include "mcTypes.h"

//Constants definitions

#define MAX_VAR_INT 5
#define MAX_VAR_LONG 10
#define TCP_TIMEOUT 500000 //ms
#define NO_COMPRESSION -1
#define MAX_PACKET 4096

#define STATUS_STATE 1
#define LOGIN_STATE 2

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
 @brief Reads the packet form socket, and parses it. Will attempt to read the packet and parse it until timeout or error, at which point it will return a NULL 
 @param socketFd the socket file descriptor
 @param compression the established compression level
 @return a parsed packet, or a nullPacket
*/
packet getPacket(int socketFd, int compression);

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
 @brief Requests the status from server after handshake indicating STATUS_STATE
 @param socketFd the socket file descriptor
 @return raw json the server responded with
*/
char* getServerStatus(int socketFd);
