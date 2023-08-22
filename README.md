# segfaultCraft

A C library for interacting with Minecraft servers

## Capabilites

For now this project is merely a C library for interacting with Minecraft servers. It is capable of logging in to a Minecraft server and parsing packets.
The idea behind the library is to abstract as much as possible, and to that end the library provides a gamestate struct that accurately reflects the sum of everything the server is sending.
For now the function for accurately editing this gamestate in real time is being developed, but the rest of the API for sending and receiving packets is ready.

## cJSON and cNBT

For the parsing of JSON files I utilize [cJSON](https://github.com/DaveGamble/cJSON) and for parsing of NBT files I utilize [cNBT](https://github.com/chmod222/cNBT). Huge thanks for all the work the respective teams have put in. For the sake of convenience I chose not to include these entire repositories as submodules and include only the parts of the source code I need.

## Game data

The client requires a Minecraft version specific json file to properly interact with the server. You can get this file here [pixlyzer-data](https://gitlab.bixilon.de/bixilon/pixlyzer-data/-/tree/master/version)
