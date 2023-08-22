#ifndef GAMESTATE_MC
#define GAMESTATE_MC

#include <stdbool.h>
#include <inttypes.h>
#include "mcTypes.h"
#include "list.h"

#include "cNBT/nbt.h"
#include "cJSON/cJSON.h"

//https://gitlab.bixilon.de/bixilon/pixlyzer-data/-/tree/master/version

//Types

//Minecraft entity
typedef struct entity{
    int id;
    UUID_t uid;
    uint8_t type;
    double x;
    double y;
    double z;
    angle_t pitch;
    angle_t yaw;
    angle_t headYaw;
    int32_t data;
    int16_t velocityX;
    int16_t velocityY;
    int16_t velocityZ;
    uint8_t animation;
} entity;

typedef enum block_faces{
    BOTTOM_FACE = 0, //-Y
    TOP_FACE = 1, //+Y
    NORTH_FACE = 2, //-Z
    SOUTH_FACE = 3, //+Z
    WEST_FACE = 4, //-X
    EAST_FACE = 5 //+X
} face_t;

typedef enum player_action_status{
    STARTED_DIGGING = 0,
    CANCELLED_DIGGING = 1,
    FINISHED_DIGGING = 2,
    DROP_ITEM_STACK = 3,
    DROP_ITEM = 4,
    SHOOT_ARROW = 5,
    SWAP_ITEM = 6
} status_t;

//Used for storing pending changes and awaiting for server confirmation
struct blockChange{
    int sequenceId;
    position location;
    face_t face;
    status_t status;
};

struct container{
    int32_t id;
    int32_t type;
    char* title;
    size_t slotCount;
    slot* slots;
    int16_t flags[9];
};

typedef enum difficulty_levels{
    PEACEFUL = 0,
    EASY = 1,
    NORMAL = 2,
    HARD = 3
} difficulty_t;

struct gamestate{
    union{
        int32_t entityId;
        uint8_t gamemode;
        uint8_t previousGamemode;
        struct container inventory;
        slot carried;
    } player;
    bool hardcore;
    identifierArray dimensions;
    struct nbt_node* registryCodec;
    identifier dimensionType; //current dimension type
    identifier dimensionName;
    int64_t hashedSeed;
    int maxPlayers;
    int viewDistance;
    int simulationDistance;
    bool reducedBugInfo;
    bool respawnScreen;
    bool debug;
    bool flat;
    bool deathLocation;
    identifier deathDimension;
    position death;
    int portalCooldown;
    bool loginPlay; //if we can send packets back during play
    listHead* entityList;
    union{
        struct blockChange* array;
        size_t len;
    } pendingChanges;
    listHead* chunks;
    difficulty_t difficulty;
    bool difficultyLocked;
    struct container* openContainer; //so in theory there can be more than one open container, but the vanilla client doesnt do that
};

//I am not certain if a list is the best choice for storing chunks, but it handles deletion and appending the best out of all the things i can think of

typedef struct block{
    double x;
    double y;
    double z;
    identifier type;
    byte stage;
    uint16_t animationData;
} block;

struct section{
    uint8_t y;
    block* blocks[16][16][16];
};

typedef struct chunk{
    int x;
    int z;
    struct section sections[24];
} chunk;

/*!
 @brief Updates the gamestate based on the packet. The packet should be non special, as in not related to the protocol
 @param input the parsed packet that will be further parsed
 @param output the gamestate that will be updated
 @param entities pointer to cJSON parsed json containing Minecraft entities definitions
 @return -1 for error and 0 for success
*/
int parsePlayPacket(packet* input, struct gamestate* output, const cJSON* entities, const identifier* globalPalette);

/*!
 @brief Initializes the gamestate struct
 @return a properly initialized struct 
*/
struct gamestate initGamestate();

#endif
