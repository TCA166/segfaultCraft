# segfaultCraft

A C library for interacting with Minecraft servers

## Capabilites

For now this project is merely a C library for interacting with Minecraft servers. It is capable of logging in to a Minecraft server and parsing packets.
The idea behind the library is to abstract as much as possible, and to that end the library provides a gamestate struct that accurately reflects the sum of everything the server is sending.
For now the function for accurately editing this gamestate in real time is being developed, but the rest of the API for sending and receiving packets is ready.

## API

The api is split into multiple parts. Each handles their own aspect of the process, and these parts are compiled and linked together into segfaultCraft.o

- **mcTypes** handles all the different types within the protocol
- **gamestateMc** provides functions for parsing packets and updating the gamestate
- **networkingMc** utilizes the two functions above to provide a good interface for interacting with servers

Aside from these three there are also minor libraries for structures like lists.

### cJSON and cNBT

For the parsing of JSON files I utilize [cJSON](https://github.com/DaveGamble/cJSON) and for parsing of NBT files I utilize [cNBT](https://github.com/chmod222/cNBT). Huge thanks for all the work the respective teams have put in. For the sake of convenience I chose not to include these entire repositories as submodules and include only the parts of the source code I need.

## Client

The client is a small test bed/implementation example for the library. If you want to look at just how much abstraction we are talking about here, or how the api looks like you can find it [here](./client.c).

### Game data

The client requires a Minecraft version specific json file to properly interact with the server. You can get such a file here [pixlyzer-data](https://gitlab.bixilon.de/bixilon/pixlyzer-data/-/tree/master/version)
