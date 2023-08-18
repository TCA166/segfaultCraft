#include <stdbool.h>
#include <inttypes.h>
#include "networkingMc.h"
#include "mcTypes.h"
#include "cNBT/nbt.h"
#include "list.h"

#ifndef GAMESTATE_MC
#define GAMESTATE_MC

typedef struct entity{
    int id;
    UUID_t uid;
    int type;
    double x;
    double y;
    double z;
    angle_t pitch;
    angle_t yaw;
    angle_t headYaw;
    int data;
    int16_t velocityX;
    int16_t velocityY;
    int16_t velocityZ;
} entity;

struct playerstate{
    int32_t entityId;
    byte gamemode;
    byte previousGamemode;
};

struct gamestate{
    struct playerstate player;
    bool hardcore : 1;
    identifierArray dimensions;
    struct nbt_node* registryCodec;
    identifier dimensionType; //current dimension type
    identifier dimensionName;
    int64_t hashedSeed;
    int maxPlayers;
    int viewDistance;
    int simulationDistance;
    bool reducedBugInfo : 1;
    bool respawnScreen : 1;
    bool debug : 1;
    bool flat : 1;
    bool deathLocation : 1;
    identifier deathDimension;
    position death;
    int portalCooldown;
    bool loginPlay : 1; //if we can send packets back during play
    listHead* entityList;
};

/*!
 @brief Updates the gamestate based on the packet. The packet should be non special, as in not related to the protocol
 @param input the parsed packet that will be further parsed
 @param output the gamestate that will be updated
 @return -1 for error and 0 for success
*/
int parsePlayPacket(packet* input, struct gamestate* output);

/*!
 @brief Initializes the gamestate struct
 @return a properly initialized struct 
*/
struct gamestate initGamestate();

#endif
