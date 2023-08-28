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

typedef enum metadataTypes{
    BYTE = 0,
    VAR_INT = 1,
    VAR_LONG = 2,
    FLOAT = 3,
    STRING = 4,
    CHAT = 5,
    OPT_CHAT = 6,
    SLOT = 7,
    BOOLEAN = 8,
    ROTATION = 9,
    POSITION = 10,
    OPT_POSITION = 11,
    DIRECTION = 12,
    OPT_UUID = 13,
    BLOCK_ID = 14,
    OPT_BLOCK_ID = 15,
    NBT = 16,
    PARTICLE = 17,
    VILLAGER_DATA = 18,
    OPT_VAR_INT = 19,
    POSE = 20,
    CAT_VARIANT = 21,
    FROG_VARIANT = 22,
    OPT_GLOBAL_POS = 23,
    PAINTING_VARIANT = 24,
    SNIFFER_STATE = 25,
    VECTOR3 = 26,
    QUATERNION = 27
} metadataType_t;

struct entityMetadata{
    metadataType_t type;
    union value{
        byte BYTE;
        int32_t VAR_INT;
        int64_t VAR_LONG;
        float FLOAT;
        char* STRING;
        char* CHAT;
        char* OPT_CHAT; //NULLABLE!
        slot SLOT;
        bool BOOLEAN;
        float ROTATION[3];
        position POSITION;
        struct optPosition{
            bool present;
            position value;
        } OPT_POSITION;
        int32_t DIRECTION;
        struct optUUID{
            bool present;
            UUID_t value;
        } OPT_UUID;
        int32_t BLOCK_ID;
        int32_t OPT_BLOCK_ID;
        nbt_node* NBT;
        void* PARTICLE;
        int32_t VILLAGER_DATA;
        int32_t OPT_VAR_INT;
        int32_t POSE;
        int32_t CAT_VARIANT;
        int32_t FROG_VARIANT;
        struct optGlobalPos{
            bool present;
            identifier dimension;
            position pos;
        } OPT_GLOBAL_POS;
        int32_t PAINTING_VARIANT;
        int32_t SNIFFER_STATE;
        float VECTOR3[3];
        float QUATERNION[4];
    } value;
};

//Minecraft entity
typedef struct entity entity;

struct entity{
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
    listHead* metadata;
    entity* linked; //The entity holding this entity
};

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
    int32_t state;
    byte stage;
    uint16_t animationData;
    blockEntity* entity; //the associated blockEntity
} block;

struct section{
    int8_t y; //The y index of this section going from -4 to plus 19
    uint16_t nonAir;
    block* blocks[16][16][16]; //NULLable array of blocks, NULL meaning it's an air block
    identifier biome[64]; //Encoded biome regions, each having a size of 4x4x4
};

//A Minecraft chunk column, consisting of a maximum of 24 sections
typedef struct chunk{
    int32_t x;
    int32_t z;
    struct section sections[24];
} chunk;

//Minecraft gameplay difficulty
typedef enum difficulty_levels{
    PEACEFUL = 0,
    EASY = 1,
    NORMAL = 2,
    HARD = 3
} difficulty_t;

//A generic property for tying name->value
struct property{
    char* name;
    char* value;
    char* signature;
};

struct genericPlayer{
    char* name;
    UUID_t id;
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
    struct player{ //data on our player
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
        chunk* currentChunk;
    } player;
    int64_t worldAge;
    int64_t timeOfDay;
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
    struct pendingChanges{ //Array of changes that have been sent to the server, but haven't been acknowledged by the server
        struct blockChange* array;
        size_t len;
    } pendingChanges;
    listHead* chunks;
    difficulty_t difficulty;
    bool difficultyLocked;
    struct container* openContainer; //so in theory there can be more than one open container, but the vanilla client doesn't do that
    struct eventHandlers{ //Every event handler must return an int, with negative values indicating errors and set errno
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
    struct worldBorder{
        double X;
        double Z;
        double diameter;
        int64_t speed;
        int32_t portalBoundary;
        int32_t warning;
        int32_t warningT;
    } worldBorder; 
    struct feature_flags{ //Flags that determine what vanilla content is present
        size_t count;
        identifier* flags;
    } featureFlags;
    struct server_data{ //the data you expect to see on the server list
        char* MOTD;
        bool hasIcon;
        byteArray icon;
        bool secureChat;
    } serverData;
    struct player_info{ //the kind of data you expect to see on the player list
        int32_t number;
        listHead* players;
    } playerInfo;
    struct default_spawn_position{
        position location;
        float angle;
    } defaultSpawnPosition;
};

//Struct that contains all necessary info that determine what game features are present in our game version
struct gameVersion{
    uint32_t protocol;
    struct palette entities;
    struct palette blockTypes;
    struct palette blockStates;
    struct palette biomes;
    struct palette airTypes;
};

//Macros

//Formula for getting the correct state from a palettedContainer that holds states
#define statesFormula(x, y, z) ((y*16*16) + (z*16) + x)

//Formula for getting the correct biome from a biome array 
#define biomeFormula(x, y, z) statesFormula(x, y, z)/64

//Foreach block in a section. Provides x, y and z
#define forBlocks() \
    for(int x = 0; x < 16; x++)\
        for(int y = 0; y < 16; y++)\
            for(int z = 0; z < 16; z++)

#define yToSection(y) (y + (4 * 16)) >> 4

//Functions

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
 @brief Frees a gamestate struct in it's entirety
*/
void freeGamestate(struct gamestate* g);

/*!
 @brief Frees the chunk and all the blocks
*/
void freeChunk(chunk* c);

/*!
 @brief handles the SYNCHRONIZE_PLAYER_POSITION packet
*/
int handleSynchronizePlayerPosition(packet* input, struct gamestate* output, int* offset);

/*!
 @brief Creates a version struct
*/
struct gameVersion* createVersionStruct(const char* versionJSON, const char* biomesJSON, uint32_t protocol);

/*!
 @brief Frees a previously created version struct
*/
void freeVersionStruct(struct gameVersion* version);

/*!
 @brief Frees a block
*/
void freeBlock(block* b);

/*!
 @brief Frees a generic player struct
*/
void freeGenericPlayer(struct genericPlayer* p);

#endif
