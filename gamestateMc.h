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

struct palette{
    identifier* palette;
    size_t sz;
};

struct statistic{
    int32_t category;
    int32_t id;
    int32_t value;
};

struct particle{
    int32_t id;
    bool longDistance;
    double x;
    double y;
    double z;
    float offsetX;
    float offsetY;
    float offsetZ;
    float maxSpeed;
    int32_t count;
    void* data;
};

typedef struct blockEntity{
    position location;
    int32_t type;
    nbt_node* tag;
} blockEntity;

//Minecraft entity
typedef struct entity{
    int id; 
    UUID_t uid; //Unique id of this entity
    uint8_t type; //id of entity class
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
    byte status;
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

//I am not certain if a list is the best choice for storing chunks, but it handles deletion and appending the best out of all the things i can think of

typedef struct block{
    double x;
    double y;
    double z;
    identifier type;
    byte stage;
    uint16_t animationData;
    blockEntity* entity;
} block;

struct section{
    uint8_t y;
    uint16_t nonAir;
    block* blocks[16][16][16];
};

typedef struct chunk{
    int32_t x;
    int32_t z;
    struct section sections[24];
} chunk;

typedef enum difficulty_levels{
    PEACEFUL = 0,
    EASY = 1,
    NORMAL = 2,
    HARD = 3
} difficulty_t;

struct property{
    char* name;
    char* value;
    char* signature;
};

struct genericPlayer{
    char* name;
    listHead* properties;
    uint8_t gamemode;
    bool listed;
    int32_t ping;
    char* displayName;
    union signature_data{
        bool present;
        UUID_t chatSessionId;
        int64_t keyExpiry;
        byteArray encodedPublicKey;
        byteArray publicKeySignature;
    } signatureData;
};

struct gamestate{
    union player{
        int32_t entityId;
        byte gamemode;
        byte previousGamemode;
        byte heldSlot;
        struct container inventory;
        slot carried;
        double X;
        double Y;
        double Z;
        float yaw;
        float pitch;
        byte flags;
        float flyingSpeed;
        float fovModifier;
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
    listHead* blockEntities;
    union pendingChanges{
        struct blockChange* array;
        size_t len;
    } pendingChanges;
    listHead* chunks;
    difficulty_t difficulty;
    bool difficultyLocked;
    struct container* openContainer; //so in theory there can be more than one open container, but the vanilla client doesn't do that
    union eventHandlers{ //Every event handler must return an int, with negative values indicating errors and set errno
        int (*spawnEntityHandler) (entity* e); //function for handling entity spawns
        int (*animationEntityHandler) (entity* e); //function for handling entity animations
        int (*entityEventHandler) (entity* e); //function for handling entity events
        int (*blockActionHandler) (block* b); //function for handling blockActions
        int (*containerHandler) (struct container* cont); //function for handling container updates. Sent when open container is changed
        int (*damageHandler) (int32_t eid, int32_t sourceType, int32_t sourceCause, int32_t sourceDirect, bool hasPosition, double sourceX, double sourceY, double sourceZ); //function for handling the damage event
        int (*explosionHandler) (double X, double Y, double Z, float strength); //function for handling explosions
        int (*generic[16]) (float value); //array of function pointers that act as generic event handlers
        int (*hurtAnimationHandler) (entity* e, float yaw);
        int (*worldBorder) (double oldDiameter);
        int (*displayStats) (struct statistic* stats, size_t num);
        int (*worldEvent) (int32_t event, position location, int32_t data, bool disableRelativeVolume);
        int (*particleSpawn) (struct particle* new);
    } eventHandlers; 
    union worldBorder{
        double X;
        double Z;
        double diameter;
        int64_t speed;
        int32_t portalBoundary;
        int32_t warning;
        int32_t warningT;
    } worldBorder;
    union feature_flags{
        size_t count;
        identifier* flags;
    } featureFlags;
    union server_data{
        char* MOTD;
        bool hasIcon;
        byteArray icon;
        bool secureChat;
    } serverData;
    union player_info{
        int32_t number;
        listHead* players;
    } playerInfo;
};

struct gameVersion{
    const cJSON* entities;
    const struct palette* blocks;
};

/*!
 @brief Updates the gamestate based on the packet. The packet should be non special, as in not related to the protocol. Packets that require a response will not get that response, and ideally should be handled separately.
 @param input the parsed packet that will be further parsed
 @param output the gamestate that will be updated
 @param version pointer to a struct that defines all game version dependant constants
 @return -1 for error and 0 for success
*/
int parsePlayPacket(packet* input, struct gamestate* output, const struct gameVersion* version);

/*!
 @brief Initializes the gamestate struct
 @return a properly initialized struct 
*/
struct gamestate initGamestate();

/*!
 @brief Frees the chunk and all the blocks
*/
void freeChunk(chunk* c);

/*!
 @brief handles the SYNCHRONIZE_PLAYER_POSITION packet
*/
int handleSynchronizePlayerPosition(packet* input, struct gamestate* output, int* offset);

#endif
