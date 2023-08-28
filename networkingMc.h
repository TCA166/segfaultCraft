#ifndef TCP_TIMEOUT

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>
#include "mcTypes.h"
#include "gamestateMc.h"

//Constants definitions

#define TCP_TIMEOUT 500000 //ms
#define NO_COMPRESSION -1
#define MAX_PACKET 4096

#define STATUS_STATE 1
#define LOGIN_STATE 2

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
 @param port the port that we are connected to
 @param nextState what will be requested next
 @return the number of bytes written, EOF or -1 for error
*/
ssize_t handshake(int socketFd, const char* host, int32_t protocol, uint16_t port, int32_t nextState);

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

/*!
 @brief Performs the login process
 @param socketFd the socket file descriptor
 @param response pointer to the most recent response in login state, gets updated to the most recent received packet
 @param given the current player UUID, gets updated to the UUID provided by the server
 @param username the player username
 @param compression pointer to the current compression level
 @return the result of the process. Negative values indicate errors, positive values indicates graceful disconnects
*/
int loginState(int socketFd, packet* response, UUID_t* given, const char* username, int* compression);

/*!
 @brief Keeps the current struct updated based on input from the server. Ideally run this in a thread.
 @param current struct containing the current gamestate
 @param response the unparsed packet from the login state
 @param socketFd the file descriptor of the socket used for communication with the server
 @param compression the established compression level
 @param thisVersion a previously created version struct
 @return the result of the process. Negative values indicate errors. 0 indicates server ordered disconnect
*/
int playState(struct gamestate* current, packet response, int socketFd, int compression, const struct gameVersion* thisVersion);

#endif
