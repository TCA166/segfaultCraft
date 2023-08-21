# segfaultCraft

A C library for interacting with Minecraft servers

## Capabilites

For now this project is merely a C library for interacting with Minecraft servers. It is capable of logging in to a Minecraft server and parsing packets.
The idea behind the library is to abstract as much as possible, and to that end the library provides a gamestate struct that accurately reflects the sum of everything the server is sending.
For now the function for accurately editing this gamestate in real time is being developed, but the rest of the API for sending and receiving packets is ready.
