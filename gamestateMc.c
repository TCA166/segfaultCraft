#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"
#include "cJSON/cJSON.h"

/*!
 @brief Gets the id of the entity with the given name
 @param version the current version struct
 @param name the name of the entity
*/
static int getEntityId(const struct gameVersion* version, const char* name);

/*!
 @brief Gets the double pointer to the block with the given position within the gamestate
 @param current the currently worked on gamestate
 @param pos the block position
 @return NULL or the double pointer
*/
static block** getBlock(struct gamestate* current, position pos);

/*!
 @brief Gets the entity with the given eid from the linked list
 @param current the gamestate from which we want to get the entity
 @param eid the id of the entity
 @return NULL or a reference to the entity
*/
static entity* getEntity(struct gamestate* current, int32_t eid);

/*!
 @brief Parses raw bytes into an entity metadata object
 @param input the raw bytes
 @param offset the pointer to the current pointer withing input
 @return a new entity metadata object
*/
static struct entityMetadata* parseEntityMetadata(byte* input, int* offset);

/*!
 @brief Removes the chunk from the chunk list, and frees it's contents
 @param el the list element the chunk is attached to
*/
static void unloadChunk(listEl* el);

/*!
 @brief Little convenience function that combines malloc and memcpy that effectively does a shallow copy
 @param value the value to copy
 @param size the size to allocate and copy
 @return a pointer to a new buffer with the same value
*/
void* mCpyAlloc(const void* value, size_t size);

/*!
 @brief Gets the chunk from the gamestate 
*/
static chunk* getChunk(const struct gamestate* current, int32_t chunkX, int32_t chunkZ);

/*!
 @brief Checks if the given type is air
*/
static bool isAir(const struct gameVersion* current, const identifier type);

/*!
 @brief Allocates and initializes a new entity object
*/
entity* initEntity();

/*!
 @brief Frees an entity
*/
static void freeEntity(entity* e);

static inline void updateEntityPosition(entity* e, const byte* buff, int* index);

static inline void updateEntityRotation(entity* e, const byte* buff, int* index);

/*!
 @brief Frees a block
*/
static void freeBlock(block* b);

/*!
 @brief Frees a generic player struct
*/
static void freeGenericPlayer(struct genericPlayer* p);

static void freeEntityAttribute(struct entityAttribute* eA);

#define event(gamestate, name, ...) \
    if(gamestate->eventHandlers.name != NULL){ \
        int result = (*gamestate->eventHandlers.name)(__VA_ARGS__); \
        if(result < 0){ \
            return result; \
        } \
    } 

//Magic numbers

#define FLAGS_X 0x01
#define FLAGS_Y 0x02
#define FLAGS_Z 0x04
#define FLAGS_Y_ROT 0x08
#define FLAGS_X_ROT 0x10

#define INFO_ADD_PLAYER 0x01
#define INFO_INIT_CHAT 0x02
#define INFO_GAMEMODE 0x04
#define INFO_UPDATE 0x08
#define INFO_PING 0x10
#define INFO_DISPLAY 0x20

#define STATES_NUM 4096

#define biomePaletteThreshold 6
#define blockPaletteThreshold 9

#define mcAirClass "AirBlock"

/*Minecraft does this sometimes where they will get a value, and in order to limit it they will multiply it so that the desired maximum value == SHORT_MAX
Call me cynical but I would use a int8 for such a thing*/

//Macro for getting the change value from a limited short
#define deltaShortFormula(s) ((int16_t)s/(4096))

#define transplantBiomes(section, biomes, version) \
    if(biomes.states == NULL){ \
        for(int i = 0; i < 64; i++){ \
            section->biome[i] = version->biomes.palette[*biomes.palette]; \
        } \
    } \
    else{ \
        for(int i = 0; i < 64; i++){ \
            int32_t id = biomes.states[i]; \
            if(biomes.palette != NULL){ \
                id = biomes.palette[id]; \
            } \
            section->biome[i] = version->biomes.palette[id]; \
        } \
    }

int parsePlayPacket(packet* input, struct gamestate* output, const struct gameVersion* version){
    int offset = 0;
    switch(input->packetId){
        case SPAWN_ENTITY:{
            entity* e = initEntity();
            e->id = readVarInt(input->data, &offset);
            e->uid = readUUID(input->data, &offset);
            e->type = readVarInt(input->data, &offset);
            e->x = readBigEndianDouble(input->data, &offset);
            e->y = readBigEndianDouble(input->data, &offset);
            e->z = readBigEndianDouble(input->data, &offset);
            e->pitch = readByte(input->data, &offset);
            e->yaw = readByte(input->data, &offset);
            e->headYaw = readByte(input->data, &offset);
            e->data = readVarInt(input->data, &offset);
            e->velocityX = (float)readBigEndianShort(input->data, &offset) / 8000.0;
            e->velocityY = (float)readBigEndianShort(input->data, &offset) / 8000.0;
            e->velocityZ = (float)readBigEndianShort(input->data, &offset) / 8000.0;
            e->linked = NULL;
            addElement(output->entityList, (void*)e);
            event(output, spawnEntityHandler, e)
            break;
        }
        case SPAWN_EXPERIENCE_ORB:{
            entity* exp = initEntity();
            exp->id = readVarInt(input->data, &offset);
            exp->type = getEntityId(version, "minecraft:experience_orb");
            exp->x = readBigEndianDouble(input->data, &offset);
            exp->y = readBigEndianDouble(input->data, &offset);
            exp->z = readBigEndianDouble(input->data, &offset);
            exp->data = (int32_t)readBigEndianShort(input->data, &offset);
            addElement(output->entityList, (void*)exp);
            event(output, spawnEntityHandler, exp)
            break;
        }
        case SPAWN_PLAYER:{
            entity* player = initEntity();
            player->id = readVarInt(input->data, &offset);
            player->uid = readUUID(input->data, &offset);
            player->type = getEntityId(version, "minecraft:player");
            player->x = readBigEndianDouble(input->data, &offset);
            player->y = readBigEndianDouble(input->data, &offset);
            player->z = readBigEndianDouble(input->data, &offset);
            player->yaw = readByte(input->data, &offset);
            player->pitch = readByte(input->data, &offset);
            addElement(output->entityList, (void*)player);
            event(output, spawnEntityHandler, player);
            break;
        }
        case ENTITY_ANIMATION:{
            int32_t eid = readVarInt(input->data, &offset);
            byte animation = readByte(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                e->animation = animation;
                event(output, animationEntityHandler, e)
            }
            break;
        }
        case AWARD_STATISTICS:{
            size_t num = (size_t)readVarInt(input->data, &offset);
            struct statistic* stats = calloc(num, sizeof(struct statistic));
            for(int i = 0; i < num; i++){
                stats[i].category = readVarInt(input->data, &offset);
                stats[i].id = readVarInt(input->data, &offset);
                stats[i].value = readVarInt(input->data, &offset);
            }
            event(output, displayStats, stats, num);
            free(stats);
            break;
        }
        case ACKNOWLEDGE_BLOCK_CHANGE:{
            int32_t sequenceChange = readVarInt(input->data, &offset);
            for(int i = 0; i < output->pendingChanges.len; i++){
                struct blockChange* change = output->pendingChanges.array + i;
                if(change->sequenceId == sequenceChange){
                    //TODO: handle rest of the cases
                    if(change->status == FINISHED_DIGGING){
                        block** b = getBlock(output, change->location);
                        if(b != NULL){
                            free(*b);
                            *b = NULL;
                        }
                    }
                }
            }
            break;
        }
        case SET_BLOCK_DESTROY_STAGE:{
            (void)readVarInt(input->data, &offset);
            position location = (position)readBigEndianLong(input->data, &offset);
            byte stage = readByte(input->data, &offset);
            block** b = getBlock(output, location);
            if(b != NULL){
                if(stage < 10){
                    (*b)->stage = stage;
                }
                else{
                    free(*b);
                    *b = NULL;
                }
            }
            break;
        }
        case BLOCK_ENTITY_DATA:{
            blockEntity* bEnt = malloc(sizeof(blockEntity));
            bEnt->location = readBigEndianLong(input->data, &offset);
            bEnt->type = readVarInt(input->data, &offset);
            size_t sz = nbtSize(input->data + offset, false);
            bEnt->tag = nbt_parse(input->data + offset, sz);
            block** b = getBlock(output, bEnt->location);
            if(b != NULL){
                (*b)->entity = bEnt;
            }
            break;
        }
        case BLOCK_ACTION:{
            position location = (position)readBigEndianLong(input->data, &offset);
            uint16_t animationData = readBigEndianShort(input->data, &offset);
            block** b = getBlock(output, location);
            if(b != NULL){
                (*b)->animationData = animationData;
                event(output, blockActionHandler, *b)
            }
            break;
        }
        case BLOCK_UPDATE:{
            position location = (position)readBigEndianLong(input->data, &offset);
            int32_t blockId = readVarInt(input->data, &offset);
            block** b = getBlock(output, location);
            if(b != NULL){
                (*b)->type = version->blockTypes.palette[blockId];
            }
            break;
        }
        case BOSS_BAR:{
            //TODO: implement this
            break;
        }
        case CHANGE_DIFFICULTY:{
            output->difficulty = readByte(input->data, &offset);
            output->difficultyLocked = readBool(input->data, &offset);
            break;
        }
        case CHUNK_BIOMES:{
            int32_t num = readVarInt(input->data, &offset);
            while(num > 0){ //foreach chunk in packet
                int32_t chunkX = readBigEndianInt(input->data, &offset);
                int32_t chunkZ = readBigEndianInt(input->data, &offset);
                int32_t limit = readVarInt(input->data, &offset) + offset;
                chunk* ourChunk = getChunk(output, chunkX, chunkZ);
                int chunkOffset = 0;
                for(int i = 0; i < 24; i++){
                    if(chunkOffset >= limit){
                        break;
                    }
                    struct section* s = ourChunk->sections + i;
                    palettedContainer biomes = readPalettedContainer(input->data, &offset, biomePaletteLowest, biomePaletteThreshold, version->biomes.sz);
                    transplantBiomes(s, biomes, version)
                }
                num--;
            }
            break;
        }
        case CLEAR_TITLES:{
            //TODO: finish and figure out what is this for
            break;
        }
        case COMMAND_SUGGESTIONS_RESPONSE:{
            //TODO: handle
            break;
        }
        case COMMANDS:{
            //TODO: handle
            break;
        }
        case CLOSE_CONTAINER:{
            //there is a given window ID; might be worth implementing a system allowing for mutiple open windows?
            output->openContainer = NULL;
            event(output, containerHandler, NULL)
            break;
        }
        case SET_CONTAINER_CONTENT:{
            byte windowId = readByte(input->data, &offset);
            (void)readVarInt(input->data, &offset);
            int32_t count = readVarInt(input->data, &offset);
            slot* slots = calloc(count, sizeof(slot));
            short n = 0;
            for(short i = count; i > 0; i--){
                slots[n] = readSlot(input->data, &offset);
            }
            output->player.carried = readSlot(input->data, &offset);
            if(windowId == 0){ //we are dealing with player inventory
                output->player.inventory.slotCount = count;
                free(output->player.inventory.slots);
                output->player.inventory.slots = slots;
                output->player.inventory.id = 0;
            }
            else if(output->openContainer != NULL && output->openContainer->id == windowId){
                output->openContainer->slotCount = count;
                free(output->openContainer->slots);
                output->openContainer->slots = slots;
                event(output, containerHandler, output->openContainer);
            }
            else{
                free(slots);
            }
            break;
        }
        case SET_CONTAINER_PROPERTY:{
            byte windowId = readByte(input->data, &offset);
            int16_t property = readBigEndianShort(input->data, &offset);
            int16_t value = readBigEndianShort(input->data, &offset);
            if(output->openContainer != NULL && output->openContainer->id == windowId){
                output->openContainer->flags[property] = value;
                event(output, containerHandler, output->openContainer);
            }
            break;
        }
        case SET_CONTAINER_SLOT:{
            byte windowId = readByte(input->data, &offset);
            (void)readVarInt(input->data, &offset);
            int16_t slotId = readBigEndianShort(input->data, &offset);
            slot data = readSlot(input->data, &offset);
            if(slotId == 0){
                output->player.inventory.slots[slotId] = data;
            }
            else if(output->openContainer != NULL && output->openContainer->id == windowId){
                output->openContainer->slots[slotId] = data;
                event(output, containerHandler, output->openContainer);
            }
            break;
        }
        case SET_COOLDOWN:{
            int32_t id = readVarInt(input->data, &offset);
            output->player.inventory.slots[id].cooldown = readVarInt(input->data, &offset);
            break;
        }
        case CHAT_SUGGESTIONS:{
            //Unused by the notchian server
            break;
        }
        case PLUGIN_MESSAGE:{
            //TODO: handle
            break;
        }
        case DAMAGE_EVENT:{
            int32_t entityId = readVarInt(input->data, &offset);
            int32_t sourceType = readVarInt(input->data, &offset); //What is this for?
            int32_t sourceCause = readVarInt(input->data, &offset);
            int32_t sourceDirect = readVarInt(input->data, &offset);
            bool hasPosition = readBool(input->data, &offset);
            double sourceX, sourceY, sourceZ; //do not read unless hasPosition == true
            if(hasPosition){
                sourceX = readBigEndianDouble(input->data, &offset);
                sourceY = readBigEndianDouble(input->data, &offset);
                sourceZ = readBigEndianDouble(input->data, &offset);
            }
            event(output, damageHandler, entityId, sourceType, sourceCause, sourceDirect, hasPosition, sourceX, sourceY, sourceZ)
            break;
        }
        case DELETE_MESSAGE:{
            //TODO: implement alongside the rest of the chat packets
            break;
        }
        case DISGUISED_CHAT_MESSAGE:{
            //TODO: implement alongside the rest of the chat system
            break;
        }
        case ENTITY_EVENT:{
            int32_t id = readBigEndianInt(input->data, &offset);
            listEl* el = output->entityList->first;
            while(el != NULL){
                entity* e = (entity*)el->value;
                if(e != NULL && e->id == id){
                    e->status = readByte(input->data, &offset);
                    event(output, entityEventHandler, e);
                }
                el = el->next;
            }
            break;
        }
        case EXPLOSION:{
            double X = readBigEndianDouble(input->data, &offset);
            double Y = readBigEndianDouble(input->data, &offset);
            double Z = readBigEndianDouble(input->data, &offset);
            float strength = readBigEndianFloat(input->data, &offset);
            //delete the blocks
            int32_t count = readVarInt(input->data, &offset);
            while(count > 0){
                int8_t Xoff = (int8_t)readByte(input->data, &offset);
                int8_t Yoff = (int8_t)readByte(input->data, &offset);   
                int8_t Zoff = (int8_t)readByte(input->data, &offset);
                block** b = getBlock(output, toPosition((X + Xoff), (Y + Yoff), (Z + Zoff)));
                if(*b != NULL){
                    free(*b);
                    *b = NULL;
                }
                count--;
            }
            output->player.X += readBigEndianFloat(input->data, &offset);
            output->player.Y += readBigEndianFloat(input->data, &offset);
            output->player.Z += readBigEndianFloat(input->data, &offset);
            event(output, explosionHandler, X, Y, Z, strength)
            break;
        }
        case UNLOAD_CHUNK:{
            int32_t X = readBigEndianInt(input->data, &offset);
            int32_t Z = readBigEndianInt(input->data, &offset);
            listEl* el = output->chunks->first;
            while(el != NULL){
                if(((chunk*)el->value)->x == X && ((chunk*)el->value)->z == Z){
                    unloadChunk(el);
                }
                el = el->next;
            }
            break;
        }
        case GAME_EVENT:{
            byte eventId = readByte(input->data, &offset);
            float value = readBigEndianFloat(input->data, &offset);
            if(output->eventHandlers.generic[eventId] != NULL){
                event(output, generic[eventId], value)
            }
            break;
        }
        case OPEN_HORSE_SCREEN:{
            byte windowId = readByte(input->data, &offset);
            int32_t slotCount = readVarInt(input->data, &offset);
            int32_t eid = readBigEndianInt(input->data, &offset);
            entity* horse = getEntity(output, eid);
            if(horse == NULL){
                break;
            }
            //TODO:implement check if actually is horse
            output->openContainer->id = windowId;
            output->openContainer->slotCount = slotCount;
            free(output->openContainer->slots);
            output->openContainer->slots = NULL;
            output->openContainer->title = NULL;
            output->openContainer->type = 1; //TODO: change to actual value
            event(output, containerHandler, output->openContainer)
            break;
        }
        case HURT_ANIMATION:{
            int32_t eid = readVarInt(input->data, &offset);
            float yaw = readBigEndianFloat(input->data, &offset);
            event(output, hurtAnimationHandler, getEntity(output, eid), yaw)
            break;
        }
        case INITIALIZE_WORLD_BORDER:{
            output->worldBorder.X = readBigEndianDouble(input->data, &offset);
            output->worldBorder.Z = readBigEndianDouble(input->data, &offset);
            double oldDiameter = readBigEndianDouble(input->data, &offset);
            output->worldBorder.diameter = readBigEndianDouble(input->data, &offset);
            output->worldBorder.speed = readVarLong(input->data, &offset);
            output->worldBorder.portalBoundary = readVarInt(input->data, &offset);
            output->worldBorder.warning = readVarInt(input->data, &offset);
            output->worldBorder.warningT = readVarInt(input->data, &offset);
            event(output, worldBorder, oldDiameter)
            break;
        }
        case CHUNK_DATA_AND_UPDATE_LIGHT:{
            chunk* newChunk = malloc(sizeof(chunk));
            newChunk->x = readBigEndianInt(input->data, &offset);
            newChunk->z = readBigEndianInt(input->data, &offset);
            //we skip the nbt tag
            size_t sz = nbtSize(input->data + offset, false);
            //nbt_node* heightmaps = nbt_parse(input->data + offset, sz);
            offset += sz;
            //here instead of needlessly slowing down the execution I forego using readByteArray
            size_t byteArrayLimit = readVarInt(input->data, &offset) + offset;
            //now we need to parse chunk data
            for(int i = 0; i < 24; i++){ //foreach section
                if(offset >= byteArrayLimit){ //if there are less than 24 section we need to detect that
                    break;
                }
                struct section s = {};
                s.y = i - 4;
                s.nonAir = readBigEndianShort(input->data, &offset);
                palettedContainer blocks = readPalettedContainer(input->data, &offset, blockPaletteLowest, blockPaletteThreshold, version->blockStates.sz);
                if(blocks.palette == NULL && blocks.states == NULL){
                    return -2;
                }
                //and now foreach block
                forBlocks(){
                    //first get the state
                    uint32_t id = 0;
                    identifier type = NULL;
                    if(blocks.states != NULL){
                        id = blocks.states[statesFormula(x, y, z)];
                        if(blocks.palette != NULL){ //apply the localPalette if necessary
                            id = blocks.palette[id];
                        }
                        //and then apply the global palette and boom type gotten
                        type = version->blockStates.palette[id];
                    }
                    else{
                        id = *blocks.palette;
                        type = version->blockStates.palette[id];
                    }
                    block* newBlock = NULL;
                    if(!isAir(version, type)){
                        //initialize the new block
                        newBlock = malloc(sizeof(block));
                        newBlock->entity = NULL;
                        newBlock->animationData = 0;
                        newBlock->stage = 0;
                        newBlock->state = id;
                        newBlock->type = type;
                        newBlock->x = x;
                        newBlock->y = y;
                        newBlock->z = z;
                        newBlock->state = id;
                    }
                    s.blocks[x][y][z] = newBlock;
                }
                free(blocks.palette);
                free(blocks.states);
                palettedContainer biomes = readPalettedContainer(input->data, &offset, biomePaletteLowest, biomePaletteThreshold, version->biomes.sz);
                transplantBiomes((&s), biomes, version)
                free(biomes.palette);
                free(biomes.states);
                newChunk->sections[i] = s;
            }
            int32_t blockEntityCount = readVarInt(input->data, &offset);
            for(int i = 0; i < blockEntityCount; i++){
                blockEntity* bEnt = malloc(sizeof(blockEntity));
                byte packedXZ = readByte(input->data, &offset);
                uint8_t secX = packedXZ >> 4;
                uint8_t secZ = packedXZ & 15;
                int16_t Y = readBigEndianShort(input->data, &offset);
                int16_t sectionId = yToSection(Y);
                bEnt->location = toPosition(secX + (newChunk->x * 16), Y, secZ + (newChunk->z * 16));
                bEnt->type = readVarInt(input->data, &offset);
                size_t sizeNbt = nbtSize(input->data + offset, false);
                bEnt->tag = nbt_parse(input->data + offset, sizeNbt);
                offset += sizeNbt;
                //and now we have to find the block in the chunk
                newChunk->sections[sectionId].blocks[secX][Y - (sectionId * 16)][secZ]->entity = bEnt;
            }
            //MAYBE: handle the light data here
            addElement(output->chunks, newChunk);
            break;
        }
        case WORLD_EVENT:{
            //Well there's no point in trying to represent sounds in the gamestate 
            event(output, worldEvent, readBigEndianInt(input->data, &offset), readBigEndianLong(input->data, &offset), readBigEndianInt(input->data, &offset), readBool(input->data, &offset));
            break;
        }
        case PARTICLE_2:{
            struct particle new;
            new.id = readVarInt(input->data, &offset);
            new.longDistance = readBool(input->data, &offset);
            new.x = readBigEndianDouble(input->data, &offset);
            new.y = readBigEndianDouble(input->data, &offset);
            new.z = readBigEndianDouble(input->data, &offset);
            new.offsetX = readBigEndianFloat(input->data, &offset);
            new.offsetY = readBigEndianFloat(input->data, &offset);
            new.offsetZ = readBigEndianFloat(input->data, &offset);
            new.maxSpeed = readBigEndianFloat(input->data, &offset);
            new.count = readBigEndianInt(input->data, &offset);
            new.data = input->data + offset;
            event(output, particleSpawn, &new);
            break;
        }
        case UPDATE_LIGHT:{
            //MAYBE: handle this
            break;
        }
        case LOGIN_PLAY:{
            output->loginPlay = true;
            output->player.entityId = readBigEndianInt(input->data, &offset);
            output->hardcore = readBool(input->data, &offset);
            output->player.gamemode = readByte(input->data, &offset);
            output->player.previousGamemode = readByte(input->data, &offset);
            {
                stringArray dimemsionNames = readStringArray(input->data, &offset);
                output->dimensions.len = dimemsionNames.len;
                output->dimensions.arr = (identifier*)dimemsionNames.arr;
            }
            {
                byte* nbt = input->data + offset;
                //oh oh O(2n)... too bad I am not writing a separate parser
                size_t sz1 = nbtSize(nbt, false);
                nbt_node* registryCodec = nbt_parse(nbt, sz1);
                if(registryCodec == NULL || registryCodec->type != TAG_COMPOUND){
                    return -1;
                }
                output->registryCodec = registryCodec;
                offset += sz1;
            }
            output->dimensionType = readString(input->data, &offset);         
            output->dimensionName = readString(input->data, &offset);   
            output->hashedSeed = readBigEndianLong(input->data, &offset);
            output->maxPlayers = readVarInt(input->data, &offset);
            output->viewDistance = readVarInt(input->data, &offset);
            output->simulationDistance = readVarInt(input->data, &offset);
            output->reducedBugInfo = readBool(input->data, &offset);
            output->respawnScreen = readBool(input->data, &offset);
            output->debug = readBool(input->data, &offset);
            output->flat = readBool(input->data, &offset);
            output->deathLocation = readBool(input->data, &offset);
            if(output->deathLocation){
                output->deathDimension = readString(input->data, &offset);
                output->death = readBigEndianLong(input->data, &offset);
            }
            //invalid read here of size 1. It seems we read after the packet. Idk why 
            output->portalCooldown = readVarInt(input->data, &offset);
            break;  
        }
        case UPDATE_ENTITY_POSITION:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                updateEntityPosition(e, input->data, &offset);
            }
            break;
        }
        case UPDATE_ENTITY_POSITION_AND_ROTATION:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                updateEntityPosition(e, input->data, &offset);
                updateEntityRotation(e, input->data, &offset);
                e->onGround = readBool(input->data, &offset);
            }
            break;
        }
        case UPDATE_ENTITY_ROTATION:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                updateEntityRotation(e, input->data, &offset);
            }
            break;
        }
        case PLAYER_ABILITIES:{
            output->player.flags = readByte(input->data, &offset);
            output->player.flyingSpeed = readBigEndianFloat(input->data, &offset);
            output->player.fovModifier = readBigEndianFloat(input->data, &offset);
            break;
        }
        case PLAYER_CHAT_MESSAGE:{
            //TODO implement this alongside the rest of the chat system
            break;
        }
        case END_COMBAT:{
            output->combat = false;
            break;
        }
        case ENTER_COMBAT:{
            output->combat = true;
            break;
        }
        case COMBAT_DEATH:{
            int32_t id = readVarInt(input->data, &offset);
            if(id != output->player.entityId){
                //something went wrong
                return -1;
            }
            char* message = readString(input->data, &offset);
            event(output, deathHandler, message);
            free(message);
        }
        case PLAYER_INFO_REMOVE:{
            int32_t num = readVarInt(input->data, &offset);
            for(int i = 0; i < num; i++){
                UUID_t uid = readUUID(input->data, &offset);
                listEl* thisEl = output->playerInfo->first;
                while(thisEl != NULL){
                    listEl* el = thisEl;
                    thisEl = thisEl->next;
                    if(((struct genericPlayer*)el->value)->id == uid){
                        unlinkElement(el);
                        freeGenericPlayer(el->value);
                        freeListElement(el, false);
                    }   
                }
            }
            break;
        }
        case PLAYER_INFO_UPDATE:{
            //this variable defines what info is appended to each player info element 
            byte actions = readByte(input->data, &offset);
            int32_t playerNumber = readVarInt(input->data, &offset);
            for(int i = 0; i < playerNumber; i++){
                struct genericPlayer* new = calloc(1, sizeof(struct genericPlayer));
                new->id = readUUID(input->data, &offset);
                if(actions & INFO_ADD_PLAYER){
                    new->name = readString(input->data, &offset);
                    new->properties = initList();
                    int32_t propertyCount = readVarInt(input->data, &offset);
                    for(int p = 0; p < propertyCount; p++){
                        struct property* prop = malloc(sizeof(struct property));
                        prop->name = readString(input->data, &offset);
                        prop->value = readString(input->data, &offset);
                        if(readBool(input->data, &offset)){
                            prop->signature = readString(input->data, &offset);
                        }
                        else{
                            prop->signature = NULL;
                        }
                        addElement(new->properties, prop);
                    }
                }
                if(actions & INFO_INIT_CHAT){
                    new->signatureData.present = readBool(input->data, &offset);
                    if(new->signatureData.present){
                        new->signatureData.chatSessionId = readUUID(input->data, &offset);
                        new->signatureData.keyExpiry = readBigEndianLong(input->data, &offset);
                        new->signatureData.encodedPublicKey = readByteArray(input->data, &offset);
                        new->signatureData.publicKeySignature = readByteArray(input->data, &offset);
                    }
                }
                if(actions & INFO_GAMEMODE){
                    new->gamemode = readVarInt(input->data, &offset);
                }
                if(actions & INFO_UPDATE){
                    new->listed = readBool(input->data, &offset);
                }
                if(actions & INFO_PING){
                    new->ping = readVarInt(input->data, &offset);
                }
                if(actions & INFO_DISPLAY){
                    if(readBool(input->data, &offset)){
                        new->displayName = readString(input->data, &offset);
                    }
                }
                addElement(output->playerInfo, new);
            }
            break;
        }
        case SYNCHRONIZE_PLAYER_POSITION:{
            //this is the non responsive version of the handler
            handleSynchronizePlayerPosition(input, output, &offset);
            break;
        }
        case UPDATE_RECIPE_BOOK:{
            //MAYBE
            break;
        }
        case REMOVE_ENTITIES:{
            int32_t num = readVarInt(input->data, &offset);
            while(num > 0){
                int32_t eid = readVarInt(input->data, &offset);
                listEl* el = output->entityList->first;
                while(el != NULL){
                    listEl* current = el;
                    el = el->next;
                    if(((entity*)current->value)->id == eid){
                        unlinkElement(current);
                        freeEntity(current->value);
                        freeListElement(current, false);
                        break;
                    }
                    
                }
                num--;
            }
            break;
        }
        case REMOVE_ENTITY_EFFECT:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                int32_t effectId = readVarInt(input->data, &offset);
                listEl* el = e->effects->first;
                while(el != NULL){
                    listEl* current = el;
                    el = el->next;
                    if(((struct entityEffect*)current->value)->effectId == effectId){
                        unlinkElement(current);
                        freeListElement(current, true);
                        break;
                    }
                }
            }
            break;
        }
        case RESOURCE_PACK:{
            char* url = readString(input->data, &offset);
            char* hash = readString(input->data, &offset);
            bool forced = readBool(input->data, &offset);
            char* promptMessage = NULL;
            if(readBool(input->data, &offset)){
                promptMessage = readString(input->data, &offset);
            }
            event(output, resourcePackHandler, url, hash, forced, promptMessage)
            free(url);
            free(hash);
            free(promptMessage);
            break;
        }
        case SET_HEAD_ROTATION:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                e->headYaw = (angle_t)readByte(input->data, &offset); 
            }
            break;
        }
        case SERVER_DATA:{
            output->serverData.MOTD = readString(input->data, &offset);
            output->serverData.hasIcon = readBool(input->data, &offset);
            output->serverData.icon = nullByteArray;
            if(output->serverData.hasIcon){
                output->serverData.icon = readByteArray(input->data, &offset);
            }
            output->serverData.secureChat = readBool(input->data, &offset);
            break;
        }
        case SET_HELD_ITEM:{
            output->player.heldSlot = readByte(input->data, &offset);
            break;
        }
        case SET_CENTER_CHUNK:{
            int32_t chunkX = readVarInt(input->data, &offset);
            int32_t chunkZ = readVarInt(input->data, &offset);
            if(output->player.currentChunk != NULL && output->player.currentChunk->x == chunkX && output->player.currentChunk->z == chunkZ){
                break;
            }
            foreachListElement(output->chunks, el){
                chunk* c = el->value;
                if(c->x == chunkX && c->z == chunkZ){
                    output->player.currentChunk = c;
                }
                if(c->x > chunkX + output->viewDistance || c->z > chunkZ + output->viewDistance){
                    unloadChunk(el);
                }
            }
            break;
        }
        case SET_DEFAULT_SPAWN_POSITION:{
            output->defaultSpawnPosition.location = readBigEndianLong(input->data, &offset);
            output->defaultSpawnPosition.angle = readBigEndianFloat(input->data, &offset);
            break;
        }
        case SET_ENTITY_METADATA:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* our = getEntity(output, eid);
            if(our != NULL){
                while(true){
                    byte index = readByte(input->data, &offset);
                    if(index == 0xff){
                        break;
                    }
                    struct entityMetadata* new = parseEntityMetadata(input->data, &offset);
                    addElement(our->metadata, new);
                }
            }
            event(output, entityEventHandler, our);
            break;            
        }
        case LINK_ENTITIES:{
            int32_t attached = readBigEndianInt(input->data, &offset);
            int32_t holding = readBigEndianInt(input->data, &offset);
            entity* attachedEntity = getEntity(output, attached);
            if(attachedEntity != NULL){
                attachedEntity->linked = getEntity(output, holding);
                event(output, entityEventHandler, attachedEntity);
            }
            break;
        }
        case SET_ENTITY_VELOCITY:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                e->velocityX = (float)readBigEndianShort(input->data, &offset) / 8000.0;
                e->velocityY = (float)readBigEndianShort(input->data, &offset) / 8000.0;
                e->velocityZ = (float)readBigEndianShort(input->data, &offset) / 8000.0;
            }
            break;
        }
        case SET_EQUIPMENT:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                byte slot = CONTINUE_BIT;
                //WARNING potential for big scary invalid reads here should say something as simple as a bit flip occur.
                while(slot & CONTINUE_BIT){
                    slot = readByte(input->data, &offset);
                    e->items[slot & SEGMENT_BITS] = readSlot(input->data, &offset);
                }
            }
            break;
        }
        case SET_EXPERIENCE:{
            output->player.experienceBar = readBigEndianFloat(input->data, &offset);
            output->player.totalExperience = readVarInt(input->data, &offset);
            output->player.level = readVarInt(input->data, &offset);
            break;
        }
        case SET_HEALTH:{
            output->player.health = readBigEndianFloat(input->data, &offset);
            output->player.food = readVarInt(input->data, &offset);
            output->player.saturation = readBigEndianFloat(input->data, &offset);
            break;
        }
        case UPDATE_TIME:{
            output->worldAge = readBigEndianLong(input->data, &offset);
            output->timeOfDay = readBigEndianLong(input->data, &offset);
            break;
        }
        case SYSTEM_CHAT_MESSAGE:{
            //TODO: handle
            break;
        }
        case UPDATE_ADVANCEMENTS:{
            //MAYBE implement this
            break;
        }
        case UPDATE_ATTRIBUTES:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                e->attributeCount = readVarInt(input->data, &offset);
                if(e->attributes != NULL){
                    for(int i = 0; i < e->attributeCount; i++){
                        freeEntityAttribute(e->attributes + i);
                    }
                    free(e->attributes);
                }
                e->attributes = calloc(e->attributeCount, sizeof(struct entityAttribute));
                for(int i = 0; i < e->attributeCount; i++){
                    struct entityAttribute new = {};
                    new.key = readString(input->data, &offset);
                    new.value = readBigEndianDouble(input->data, &offset);
                    new.modifierCount = readVarInt(input->data, &offset);
                    new.modifiers = calloc(new.modifierCount, sizeof(struct modifier));
                    for(int n = 0; n < new.modifierCount; n++){
                        struct modifier m = {};
                        m.uid = readUUID(input->data, &offset);
                        m.amount = readBigEndianDouble(input->data, &offset);
                        m.operation = readByte(input->data, &offset);
                        new.modifiers[n] = m;
                    }
                    e->attributes[i] = new;
                }
            }
            break;
        }
        case FEATURE_FLAGS:{
            int32_t count = readVarInt(input->data, &offset);
            output->featureFlags.flags = calloc(count, sizeof(identifier));
            for(int i = 0; i < count; i++){
                output->featureFlags.flags[i] = readString(input->data, &offset);
            }
            output->featureFlags.count = count;
            break;
        }
        case ENTITY_EFFECT:{
            int32_t eid = readVarInt(input->data, &offset);
            entity* e = getEntity(output, eid);
            if(e != NULL){
                struct entityEffect* new = malloc(sizeof(struct entityEffect));
                new->effectId = readVarInt(input->data, &offset);
                new->amplifier = readByte(input->data, &offset);
                new->duration = readVarInt(input->data, &offset);
                new->flags = readByte(input->data, &offset);
                new->factorDataPresent = readBool(input->data, &offset);
                if(new->factorDataPresent){
                    nbt_node* factorCodec = nbt_parse(input->data + offset, input->size - offset);
                    new->factorData.paddingDuration = nbt_find_by_name(factorCodec, "padding_duration")->payload.tag_int;
                    new->factorData.factorStart = nbt_find_by_name(factorCodec, "factor_start")->payload.tag_float;
                    new->factorData.factorTarget = nbt_find_by_name(factorCodec, "factor_target")->payload.tag_float;
                    new->factorData.factorCurrent = nbt_find_by_name(factorCodec, "factor_current")->payload.tag_float;
                    new->factorData.effectChangedTimestamp = nbt_find_by_name(factorCodec, "effect_changed_timestamp")->payload.tag_int;
                    new->factorData.factorPreviousFrame = nbt_find_by_name(factorCodec, "factor_previous_frame")->payload.tag_float;
                    new->factorData.hadEffectLastTick = (bool)nbt_find_by_name(factorCodec, "had_effect_last_tick")->payload.tag_byte;
                    nbt_free(factorCodec);
                }
            }
            break;
        }
        case UPDATE_RECIPES:{
            //MAYBE: is this necessary?
            break;
        }
        case UPDATE_TAGS:{
            //int32_t count = readVarInt(input->data, &offset);
            //MAYBE: what is this for?
            break;
        }
        default:{
            errno = EINVAL;
            return -1;
            break;
        }
    }
    return 0;
}

struct gamestate initGamestate(){
    struct gamestate g = {};
    memset(&g, 0, sizeof(struct gamestate));
    g.entityList = initList();
    g.chunks = initList();
    g.playerInfo = initList();
    return g;
}

void freeGamestate(struct gamestate* g){
    freeList(g->entityList, (void(*)(void*))freeEntity);
    freeList(g->chunks, (void(*)(void*))freeChunk);
    free(g->deathDimension);
    free(g->dimensionName);
    for(int i = 0; i < g->dimensions.len; i++){
        free(g->dimensions.arr[i]);
    }
    free(g->dimensions.arr);
    free(g->dimensionType);
    for(int i = 0; i < g->featureFlags.count; i++){
        free(g->featureFlags.flags[i]);
    }
    free(g->featureFlags.flags);
    if(g->openContainer != NULL){
        free(g->openContainer->slots);
        free(g->openContainer->title);
    }
    free(g->openContainer);
    free(g->pendingChanges.array);
    free(g->player.inventory.slots);
    free(g->player.inventory.title);
    freeList(g->playerInfo, (void(*)(void*))freeGenericPlayer);
    nbt_free(g->registryCodec);
    free(g->serverData.icon.bytes);
    free(g->serverData.MOTD);
}

//O(n) complexity, but still should be fast-ish with just 1000 elements
//LATER:implementing a hashtable is something that can be done later
static int getEntityId(const struct gameVersion* version, const char* name){
    for(int i = 0; i < version->entities.sz; i++){
        if(strcmp(name, version->entities.palette[i]) == 0){
            return i;
        }
    }
    return -1;
}

static block** getBlock(struct gamestate* current, position pos){
    int x = positionX(pos);
    int y = positionY(pos);
    int z = positionZ(pos);
    short chunkX = x >> 4;
    short chunkZ = z >> 4;
    short sectionId = yToSection(y);
    listEl* e = current->chunks->first;
    chunk* c = NULL;
    while(e != NULL){
        chunk* ch = (chunk*)e->value;
        if(ch->x == chunkX && ch->z == chunkZ){
            c = ch;
            break;
        }
        e = e->next;
    }
    if(c == NULL){
        return NULL;
    }
    x -= chunkX * 16;
    z -= chunkZ * 16;
    y -= sectionId * 16;
    return &(c->sections[sectionId].blocks[x][y][z]);
}

void freeChunk(chunk* c){
    for(uint8_t s = 0; s < 24; s++){
        if(c->sections[s].nonAir > 0){
            for(uint8_t x = 0; x < 16; x++){
                for(uint8_t y = 0; y < 16; y++){
                    for(uint8_t z = 0; z < 16; z++){
                        freeBlock(c->sections[s].blocks[x][y][z]);
                    }
                }
            }
        }
    }
    free(c);
}

int handleSynchronizePlayerPosition(packet* input, struct gamestate* output, int* offset){
    if(input->packetId != SYNCHRONIZE_PLAYER_POSITION){
        errno = EINVAL;
        return -1;
    }
    double X = readBigEndianDouble(input->data, offset);
    double Y = readBigEndianDouble(input->data, offset);
    double Z = readBigEndianDouble(input->data, offset);
    float yaw = readBigEndianFloat(input->data, offset);
    float pitch = readBigEndianFloat(input->data, offset);
    byte flags = readByte(input->data, offset);
    if(flags & FLAGS_X){
        output->player.X += X;
    }
    else{
        output->player.X = X;
    }
    if(flags & FLAGS_Y){
        output->player.Y += Y;
    }
    else{
        output->player.Y = Y;
    }
    if(flags & FLAGS_Z){
        output->player.Z += Z;
    }
    else{
        output->player.Z = Z;
    }
    if(flags & FLAGS_X_ROT){
        output->player.yaw += yaw;
    }
    else{
        output->player.yaw = yaw;
    }
    if(flags & FLAGS_X_ROT){
        output->player.pitch += pitch;
    }
    else{
        output->player.pitch = pitch;
    }
    return 0;
}

static entity* getEntity(struct gamestate* current, int32_t eid){
    listEl* el = current->entityList->first;
    while(el != NULL){
        entity* e = (entity*)el->value;
        if(e != NULL && e->id == eid){
            return e;
        }
        el = el->next;
    }
    return NULL;
}

static void unloadChunk(listEl* el){
    chunk* c = el->value;
    if(c != NULL){
        freeChunk(c);
        unlinkElement(el);
        freeListElement(el, false);
    }
}

static struct entityMetadata* parseEntityMetadata(byte* input, int* offset){
    struct entityMetadata* new = malloc(sizeof(struct entityMetadata));
    new->type = readVarInt(input, offset);
    switch(new->type){
        case BYTE:{
            new->value.BYTE = readByte(input, offset);
            break;
        }
        case VAR_INT:{
            new->value.VAR_INT = readVarInt(input, offset);
            break;
        }
        case VAR_LONG:{
            new->value.VAR_LONG = readVarLong(input, offset);
            break;
        }
        case FLOAT:{
            new->value.FLOAT = readBigEndianFloat(input, offset);
            break;
        }
        case STRING:{
            new->value.STRING = readString(input, offset);
            break;
        }
        case CHAT:{
            new->value.CHAT = readString(input, offset);
            break;
        }
        case OPT_CHAT:{
            bool present = readBool(input, offset);
            if(present){
                new->value.OPT_CHAT = readString(input, offset);
            }
            else{
                new->value.OPT_CHAT = NULL;
            }
            break;
        }
        case SLOT:{
            new->value.SLOT = readSlot(input, offset);
            break;
        }
        case BOOLEAN:{
            new->value.BOOLEAN = readBool(input, offset);
            break;
        }
        case ROTATION:{
            for(int i = 0; i < 3; i++){
                new->value.ROTATION[i] = readBigEndianFloat(input, offset);
            }
            break;
        }
        case POSITION:{
            new->value.POSITION = readBigEndianLong(input, offset);
            break;
        }
        case OPT_POSITION:{
            new->value.OPT_POSITION.present = readBool(input, offset);
            if(new->value.OPT_POSITION.present){
                new->value.OPT_POSITION.value = readBigEndianLong(input, offset);
            }
            break;
        }
        case DIRECTION:{
            new->value.DIRECTION = readVarInt(input, offset);
            break;
        }
        case OPT_UUID:{
            new->value.OPT_UUID.present = readBool(input, offset);
            if(new->value.OPT_UUID.present){
                new->value.OPT_UUID.value = readUUID(input, offset);
            }
            break;
        }
        case BLOCK_ID:{
            new->value.BLOCK_ID = readVarInt(input, offset);
            break;
        }
        case OPT_BLOCK_ID:{
            new->value.OPT_BLOCK_ID = readVarInt(input, offset);
            break;
        }
        case NBT:{
            size_t sz = nbtSize(input + *offset, false);
            new->value.NBT = nbt_parse(input + *offset, sz);
            offset += sz;
            break;
        }
        case PARTICLE:{
            //MAYBE implement the rest of the particle data writing
            new->value.PARTICLE = readByte(input, offset);
            break;
        }
        case VILLAGER_DATA:{
            new->value.VILLAGER_DATA = readVarInt(input, offset);
            break;
        }
        case OPT_VAR_INT:{
            new->value.OPT_VAR_INT = readVarInt(input, offset);
            break;
        }
        case POSE:{
            new->value.POSE = readVarInt(input, offset);
            break;
        }
        case CAT_VARIANT:{
            new->value.CAT_VARIANT = readVarInt(input, offset);
            break;
        }
        case FROG_VARIANT:{
            new->value.FROG_VARIANT = readVarInt(input, offset);
            break;
        }
        case OPT_GLOBAL_POS:{
            new->value.OPT_GLOBAL_POS.present = readBool(input, offset);
            if(new->value.OPT_GLOBAL_POS.present){
                new->value.OPT_GLOBAL_POS.dimension = readString(input, offset);
                new->value.OPT_GLOBAL_POS.pos = readBigEndianLong(input, offset);
            }
            break;
        }
        case PAINTING_VARIANT:{
            new->value.PAINTING_VARIANT = readVarInt(input, offset);
            break;
        }
        case SNIFFER_STATE:{
            new->value.SNIFFER_STATE = readVarInt(input, offset);
            break;
        }
        case VECTOR3:{
            for(int i = 0; i < 3; i++){
                new->value.VECTOR3[i] = readBigEndianFloat(input, offset);
            }
            break;
        }
        case QUATERNION:{
            for(int i = 0; i < 4; i++){
                new->value.QUATERNION[i] = readBigEndianFloat(input, offset);
            }
            break;
        }
    }
    return new;
}

static bool isAir(const struct gameVersion* current, const identifier type){
    bool result = false;
    for(int i = 0; i < current->airTypes.sz; i++){
        result |= strcmp(current->airTypes.palette[i], type) == 0;
    }
    return result;
}

struct gameVersion* createVersionStruct(const char* versionJSON, const char* biomesJSON, uint32_t protocol){
    //first we need to parse the json
    char* jsonContents = NULL;
    long sz = 0;
    {
        FILE* json = fopen(versionJSON, "r");
        fseek(json, 0, SEEK_END);
        sz = ftell(json);
        fseek(json, 0, SEEK_SET);
        jsonContents = calloc(sz + 1, 1);
        fread(jsonContents, sz, 1, json);
        fclose(json);
    }
    cJSON* version = cJSON_ParseWithLength(jsonContents, sz);
    free(jsonContents);
    if(version == NULL){
        return NULL;
    }
    struct gameVersion* thisVersion = malloc(sizeof(struct gameVersion));
    {//create a lookup table for entities
        cJSON* entities = cJSON_GetObjectItemCaseSensitive(version, "entities");
        thisVersion->entities.sz = cJSON_GetArraySize(entities);
        thisVersion->entities.palette = calloc(thisVersion->entities.sz, sizeof(identifier));
        short num = 0;
        cJSON* child = entities->child;
        for(int i = 0; i < thisVersion->entities.sz; i++){
            cJSON* id = cJSON_GetObjectItemCaseSensitive(child, "id");
            if(id != NULL){
                thisVersion->entities.palette[id->valueint] = child->valuestring;
                num++;
            }
            child = child->next;
        }
        //downsize the array if necessary
        if(thisVersion->entities.sz != num){
            thisVersion->entities.palette = realloc(thisVersion->entities.palette, num * sizeof(identifier));
            thisVersion->entities.sz = num;
        }
    }   
    {//then we create a lookup table for blocks and states
        //first get the blocks part of the json
        const cJSON* blocks = cJSON_GetObjectItemCaseSensitive(version, "blocks");
        //then initialize the blockTypes array for writing
        thisVersion->blockTypes.sz = cJSON_GetArraySize(blocks);
        thisVersion->blockTypes.palette = calloc(thisVersion->blockTypes.sz, sizeof(identifier));
        //then blockStates
        int n = 1;
        cJSON* lastStates = NULL;
        do{ //we need to consider the possibility of the last element not having states
            lastStates = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(blocks, thisVersion->blockTypes.sz - n), "states");
            n++;
        }
        while(lastStates == NULL);
        cJSON* lastState = cJSON_GetArrayItem(lastStates, cJSON_GetArraySize(lastStates) - 1);
        thisVersion->blockStates.sz = atoi(lastState->string) + 1;
        thisVersion->blockStates.palette = calloc(thisVersion->blockStates.sz, sizeof(identifier));
        thisVersion->airTypes.sz = 0; //now this is the index of the air array
        thisVersion->airTypes.palette = NULL;
        cJSON* child = blocks->child;
        for(int i = 0; i < thisVersion->blockTypes.sz; i++){
            cJSON* id = cJSON_GetObjectItemCaseSensitive(child, "id");
            char* name = mCpyAlloc(child->string, strlen(child->string) + 1);
            thisVersion->blockTypes.palette[id->valueint] = name;
            cJSON* states = cJSON_GetObjectItemCaseSensitive(child, "states");
            size_t statesSz = cJSON_GetArraySize(states);
            cJSON* state = states->child;
            for(int s = 0; s < statesSz; s++){
                thisVersion->blockStates.palette[atoi(state->string)] = name;
                state = state->next;
            }
            cJSON* class = cJSON_GetObjectItemCaseSensitive(child, "class");
            if(class != NULL && strcmp(class->valuestring, "AirBlock") == 0){
                thisVersion->airTypes.palette = realloc(thisVersion->airTypes.palette, (thisVersion->airTypes.sz + 1) * sizeof(identifier));
                thisVersion->airTypes.palette[thisVersion->airTypes.sz] = name;
                thisVersion->airTypes.sz++;
            }
            child = child->next;
        }
    }
    cJSON_Delete(version);
    thisVersion->protocol = protocol;
    char* biomesContent = NULL;
    size_t biomesContentSize = 0;
    {
        FILE* json = fopen(biomesJSON, "r");
        fseek(json, 0, SEEK_END);
        biomesContentSize = ftell(json);
        fseek(json, 0, SEEK_SET);
        biomesContent = calloc(sz + 1, 1);
        fread(biomesContent, sz, 1, json);
        fclose(json);
    }
    cJSON* biomes = cJSON_ParseWithLength(biomesContent, biomesContentSize);
    free(biomesContent);
    {
        thisVersion->biomes.sz = cJSON_GetArraySize(biomes);
        thisVersion->biomes.palette = calloc(thisVersion->biomes.sz, sizeof(identifier));
        cJSON* child = biomes->child;
        while(child != NULL){
            thisVersion->biomes.palette[cJSON_GetObjectItemCaseSensitive(child, "id")->valueint] = mCpyAlloc(child->string, strlen(child->string) + 1);
            child = child->next;
        }
    }
    cJSON_Delete(biomes);
    return thisVersion;
}

void freeVersionStruct(struct gameVersion* version){
    if(version != NULL){
        for(int i = 0; i < version->blockTypes.sz; i++){
            free(version->blockTypes.palette[i]);
        }
        free(version->blockTypes.palette);
        free(version->blockStates.palette);
        for(int i = 0; i < version->biomes.sz; i++){
            free(version->biomes.palette[i]);
        }
        free(version->biomes.palette);
        free(version->airTypes.palette);
        free(version->entities.palette);
        free(version);
    }
}

static void freeBlockEntity(blockEntity* e){
    if(e != NULL){
        nbt_free(e->tag);
        free(e);
    }
}

static void freeBlock(block* b){
    if(b != NULL){
        freeBlockEntity(b->entity);
        free(b);
    }
}

static void freeProperty(struct property* p){
    if(p != NULL){
        free(p->name);
        free(p->signature);
        free(p->value);
        free(p);
    }
}

static void freeGenericPlayer(struct genericPlayer* p){
    if(p != NULL){
        free(p->displayName);
        free(p->name);
        freeList(p->properties, (void(*)(void*))freeProperty);
        free(p);
    }
}

void* mCpyAlloc(const void* value, size_t size){
    void* res = malloc(size);
    memcpy(res, value, size);
    return res;
}

static chunk* getChunk(const struct gamestate* current, int32_t chunkX, int32_t chunkZ){
    chunk* res = NULL;
    foreachListElement(current->chunks, el){
        chunk* c = el->value;
        if(c != NULL && c->x == chunkX && c->z == chunkZ){
            res = c;
            break;
        }
    }
    return res;
}

entity* initEntity(){
    entity* new = calloc(1, sizeof(entity));
    new->metadata = initList();
    new->effects = initList();
    return new;
}

static void freeEntity(entity* e){
    if(e != NULL){
        freeList(e->metadata, free);
        for(int i = 0; i < e->attributeCount; i++){
            freeEntityAttribute(e->attributes + i);
        }
        freeList(e->effects, free);
        free(e->attributes);
        for(int i = 0; i < MAX_ENT_SLOT_COUNT; i++){
            nbt_free(e->items[i].NBT);
        }
        free(e);
    }
}

static inline void updateEntityPosition(entity* e, const byte* buff, int* index){
    e->x += (double)deltaShortFormula(readBigEndianShort(buff, index));
    e->y += (double)deltaShortFormula(readBigEndianShort(buff, index));
    e->z += (double)deltaShortFormula(readBigEndianShort(buff, index));
}

static inline void updateEntityRotation(entity* e, const byte* buff, int* index){
    e->yaw = readByte(buff, index);
    e->pitch = readByte(buff, index);
}

static void freeEntityAttribute(struct entityAttribute* eA){
    free(eA->key);
    free(eA->modifiers);
}
