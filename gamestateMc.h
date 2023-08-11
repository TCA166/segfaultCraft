#include <stdbool.h>
#include <inttypes.h>
#ifndef TCP_TIMEOUT
#include "networkingMc.h"
#endif
#ifndef SEGMENT_BITS
#include "mcTypes.h"
#endif
#include "cNBT/nbt.h"

struct playerstate{
    int32_t entityId;
    byte gamemode;
    byte previousGamemode;
};

struct gamestate{
    struct playerstate player;
    bool hardcore : 1;
    identifierArray dimensions;
    struct nbt_list* registryCodec;
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
};

/*!
 @brief Updates the gamestate based on the packet. The packet should be non special, as in not related to the protocol
 @param input the parsed packet that will be further parsed
 @param output the gamestate that will be updated
 @return -1 for error and 0 for success
*/
int parsePlayPacket(packet* input, struct gamestate* output);