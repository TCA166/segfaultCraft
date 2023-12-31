#ifndef GAMESTATE_MC
#define GAMESTATE_MC

#include <stdbool.h>
#include <inttypes.h>

#include "mcTypes.h"
#include "list.h"

#include "cNBT/nbt.h"
#include "cJSON/cJSON.h"

//https://gitlab.bixilon.de/bixilon/pixlyzer-data/-/tree/master/version

#define MAX_ENT_SLOT_COUNT 128

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
        byte PARTICLE;
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

//Attribute modifier operation
typedef enum modifierOperation{
    ADD_SUBTRACT_AMT = 0,
    ADD_SUBTRACT_PRC = 1,
    MULTIPLY_BY_PRC = 2
} modifierOperation_t;

//Modifier applied to entityAttribute
struct modifier{
    UUID_t uid;
    double amount;
    modifierOperation_t operation;
};

//An entity attribute, don't confuse with metadata
struct entityAttribute{
    identifier key;
    double value;
    size_t modifierCount;
    struct modifier* modifiers;
};

struct entityEffect{
    int32_t effectId;
    byte amplifier;
    int32_t duration;
    byte flags;
    bool factorDataPresent;
    struct factorData{ //storing a whole nbt tag here would be wasteful;
        int32_t paddingDuration;
        float factorStart;
        float factorTarget;
        float factorCurrent;
        int32_t effectChangedTimestamp;
        float factorPreviousFrame;
        bool hadEffectLastTick;
    } factorData;
};

//Minecraft entity
typedef struct entity entity;

struct entity{
    int32_t id; 
    UUID_t uid; //Unique id of this entity
    uint8_t type; //id of entity class
    double x;
    double y;
    double z;
    angle_t pitch;
    angle_t yaw;
    angle_t headYaw;
    bool onGround;
    int32_t data;
    float velocityX;
    float velocityY;
    float velocityZ;
    uint8_t animation;
    byte status;
    listHead* metadata;
    entity* linked; //The entity holding this entity
    struct entityAttribute* attributes;
    size_t attributeCount;
    slot items[MAX_ENT_SLOT_COUNT]; //MAYBE optimize the storage for items of entities
    listHead* effects;
    size_t passengerCount;
    entity** passengers;
    nbt_node* tag;
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
    int32_t x;
    int32_t y;
    int32_t z;
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
    UNDEFINED = -1,
    PEACEFUL = 0,
    EASY = 1,
    NORMAL = 2,
    HARD = 3
} difficulty_t;

//A generic property for tying player properties
struct property{
    char* name;
    char* value;
    char* signature;
};

//Predominately just player info
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

struct nbtQuery{
    int32_t id;
    blockEntity* blockEntity;
    entity* e;
};

struct bossBar{
    UUID_t barId;
    char* title;
    float health;
    int32_t color;
    int32_t division;
    byte flags;
};

struct title{
    char* text;
    int32_t fadeIn;
    int32_t stay;
    int32_t fadeOut;
};

struct commandSuggestion{
    char* match;
    char* tooltip;
};

struct previousMessage{
    int32_t index;
    byteArray signature;
};

typedef enum{
    PASS_THROUGH=0,
    FULLY_FILTERED=1,
    PARTIALLY_FILTERED=2
} filterType;

struct playerMessage{
    UUID_t sender;
    int32_t index;
    byteArray signature;
    char* message;
    int64_t timestamp;
    int64_t salt;
    size_t previousMessageCount;
    struct previousMessage* previous;
    char* unsignedContent;
    filterType filter;
    bitSet filterTypeBits;
    int32_t chatType;
    char* networkName;
    char* networkTarget;
};

//MAYBE lists replaced with hashTables

struct gamestate{
    struct player{ //data on our player
        int32_t entityId;
        difficulty_t gamemode;
        difficulty_t previousGamemode;
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
        float health; //?->20
        uint8_t food; //0<->20
        float saturation; //0<->5
        float experienceBar; //0<->1
        int32_t totalExperience;
        int32_t level;
        entity* cameraEntity;
        entity* playerEntity;
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
    listHead* pendingChanges;
    listHead* queries; //list of nbt tag queries
    listHead* chunks;
    difficulty_t difficulty;
    bool difficultyLocked;
    struct container* openContainer; //so in theory there can be more than one open container, but the vanilla client doesn't do that
    struct eventHandlers{ //Every event handler must return an int, with negative values indicating errors and set errno
        //TODO make sure this 1) covers all necessary events 2) is "compressed" enough
        int (*spawnEntityHandler) (entity* e); //function for handling entity spawns
        int (*animationEntityHandler) (entity* e); //function for handling entity animations
        int (*entityEventHandler) (entity* e); //function for handling entity events
        int (*blockActionHandler) (block* b); //function for handling blockActions
        int (*containerHandler) (struct container* cont); //function for handling container updates. Sent when open container is changed
        int (*damageHandler) (int32_t eid, int32_t sourceType, int32_t sourceCause, int32_t sourceDirect, double sourceX, double sourceY, double sourceZ); //function for handling the damage event
        int (*explosionHandler) (double X, double Y, double Z, float strength); //function for handling explosions
        int (*generic[16]) (float value); //array of function pointers that act as generic event handlers
        int (*hurtAnimationHandler) (entity* e, float yaw); //fired when an entity receives some damage
        int (*worldBorder) (double oldDiameter); //fired when the world border changes
        int (*displayStats) (struct statistic* stats, size_t num); //fired when server returns stats
        int (*worldEvent) (int32_t event, position location, int32_t data, bool disableRelativeVolume); //fired when a world event happens
        int (*particleSpawn) (struct particle* new); //fired when a new particle should be spawned
        int (*resourcePackHandler) (char* url, char hash[40], bool forced, char* promptMessage); //fired when the server informs about resource packs
        int (*deathHandler) (char* message); //fired on player death
        int (*openBook) (int32_t hand); //fired when book is opened
        int (*openSignEditor) (position location, bool isFrontText); // fired when a sign editor should be opened
        int (*soundEffect) (int32_t soundId, identifier soundName, float range, int32_t soundCategory, int32_t X, int32_t Y, int32_t Z, float volume, float pitch, int64_t seed); //fired when a sound should be played
        int (*stopSound) (byte flags, int32_t source, identifier sound); //fired when a sound should stop
        int (*pickupItem) (entity* collected, entity* collector, int32_t count); //fired when an item is picked up
        int (*commandSuggestions) (int32_t id, int32_t start, int32_t length, size_t count, struct commandSuggestion* suggestions);
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
    listHead* playerInfo;
    struct default_spawn_position{
        position location;
        float angle;
    } defaultSpawnPosition;
    bool combat;
    listHead* bossBars;
    char* subtitleText;
    struct title* currentTitle;
    listHead* playerChat;
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

#endif
