#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <math.h>

#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"
#include "cJSON/cJSON.h"

/*!
 @brief Gets the id of the entity type with the given name from the parsed JSON
 @param entities a parsed JSON object containing definitions all the different Minecraft entity types
 @param name the name of the entity
 @return the id
*/
static int getEntityId(const cJSON* entities, const char* name);

/*!
 @brief Gets the double pointer to the block with the given position within the gamestate
 @param current the currently worked on gamestate
 @param pos the block position
 @return NULL or the double pointer
*/
static block** getBlock(struct gamestate* current, position pos);

/*!
 @brief Gets the entity with the given eid from the linked list
 @param entities the linked list containing the entity
 @param eid the id of the entity
 @return NULL or a reference to the entity
*/
entity* getEntity(listHead* entities, int32_t eid);

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

#define event(gamestate, name, ...) \
    if(gamestate->eventHandlers.name != NULL){ \
        int result = (*gamestate->eventHandlers.name)(__VA_ARGS__); \
        if(result < 0){ \
            return result; \
        } \
    } 

#define forBlocks() \
    for(int x = 0; x < 16; x++)\
        for(int y = 0; y < 16; y++)\
            for(int z = 0; z < 16; z++)

#define yToSection(y) (y + (4 * 16)) >> 4

#define createLongMask(startBit, X) ((((uint64_t)1) << X) - 1) << startBit

#define statesFormula(x, y, z) (y*16*16) + (z*16) + x

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

int parsePlayPacket(packet* input, struct gamestate* output, const struct gameVersion* version){
    const int paletteThreshold = 9;
    int offset = 0;
    switch(input->packetId){
        case SPAWN_ENTITY:{
            entity* e = malloc(sizeof(entity));
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
            e->velocityX = readBigEndianShort(input->data, &offset);
            e->velocityY = readBigEndianShort(input->data, &offset);
            e->velocityZ = readBigEndianShort(input->data, &offset);
            addElement(output->entityList, (void*)e);
            event(output, spawnEntityHandler, e)
            break;
        }
        case SPAWN_EXPERIENCE_ORB:{
            entity* exp = malloc(sizeof(entity));
            exp->id = readVarInt(input->data, &offset);
            exp->type = getEntityId(version->entities, "minecraft:experience_orb");
            exp->x = readBigEndianDouble(input->data, &offset);
            exp->y = readBigEndianDouble(input->data, &offset);
            exp->z = readBigEndianDouble(input->data, &offset);
            exp->data = (int32_t)readBigEndianShort(input->data, &offset);
            addElement(output->entityList, (void*)exp);
            event(output, spawnEntityHandler, exp)
            break;
        }
        case SPAWN_PLAYER:{
            entity* player = malloc(sizeof(entity));
            player->id = readVarInt(input->data, &offset);
            player->uid = readUUID(input->data, &offset);
            player->type = getEntityId(version->entities, "minecraft:player");
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
            entity* e = getEntity(output->entityList, eid);
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
                (*b)->type = version->blocks.palette[blockId];
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
            while(num > 0){
                int32_t chunkX = readBigEndianInt(input->data, &offset);
                int32_t chunkZ = readBigEndianInt(input->data, &offset);
                byteArray chunk = readByteArray(input->data, &offset);
                int chunkOffset = 0;
                for(int i = 0; i < 24; i++){
                    if(chunkOffset >= chunk.len){
                        break;
                    }

                }
                free(chunk.bytes);
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
            entity* horse = getEntity(output->entityList, eid);
            if(horse == NULL){
                break;
            }
            const cJSON* ent = version->entities;
            while(ent != NULL){
                if(horse->type == cJSON_GetObjectItemCaseSensitive(ent, "id")->valueint){
                    if(strcmp(cJSON_GetObjectItemCaseSensitive(ent, "class")->valuestring, "HorseEntity") != 0){
                        break;
                    }
                }
                ent = ent->next;
            }
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
            event(output, hurtAnimationHandler, getEntity(output->entityList, eid), yaw)
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
                s.y = 24 - 4;
                s.nonAir = readBigEndianShort(input->data, &offset);
                byte bitsPerEntry = readByte(input->data, &offset);
                if(bitsPerEntry == 0){ //the palette contains a single value and the dataArray is empty
                    int32_t globalId = readVarInt(input->data, &offset);
                    //and this id in the global palette indicates what every block in section is
                    forBlocks(){
                        s.blocks[x][y][z] = malloc(sizeof(block));
                        memset(s.blocks[x][y][z], 0, sizeof(block));
                        s.blocks[x][y][z]->type = version->blocks.palette[globalId];
                        s.blocks[x][y][z]->x = x;
                        s.blocks[x][y][z]->y = y;
                        s.blocks[x][y][z]->z = z;
                    }
                    (void)readVarInt(input->data, &offset); //we need to offset by the data array length field
                }
                else{
                    uint32_t* localPalette = NULL;
                    size_t paletteLength = 0;
                    if(bitsPerEntry >= paletteThreshold){
                        //the ceilf(log2f(size of palette)) is the size of elements in the array
                        bitsPerEntry = (byte)ceilf(log2f((float)version->blocks.sz));
                        //and the local palette is the global palette
                    }
                    else{
                        if(bitsPerEntry < 4){
                            bitsPerEntry = 4;
                        }
                        //else we just keep the value
                        paletteLength = readVarInt(input->data, &offset);
                        localPalette = calloc(paletteLength, sizeof(uint32_t));
                        for(int n = 0; n < paletteLength; n++){
                            //FIXME: either the documentation is wrong, or the data is being sent incorrectly
                            uint32_t element = readVarInt(input->data, &offset);
                            //sanity check 1
                            if(element > version->blocks.sz){
                                errno = E2BIG;
                                return -1;
                            }
                            localPalette[n] = element;
                        }
                    }
                    //and now we need to get the states
                    uint16_t numPerLong = (uint16_t)(64/bitsPerEntry); //number of elements of the array in a single long
                    uint16_t index = 0; //the current MAIN array index
                    int32_t numLongs = readVarInt(input->data, &offset); //the number of longs the MAIN array has been split into
                    uint32_t* states = calloc(numPerLong * numLongs, sizeof(uint32_t)); //the states
                    for(int l = 0; l < numLongs; l++){//foreach Long
                        uint64_t ourLong = readBigEndianULong(input->data, &offset);
                        for(uint8_t b = 0; b < numPerLong; b++){ //foreach element in long
                            if(index >= 4096){
                                break;
                            }
                            uint16_t bits = b * bitsPerEntry;
                            uint32_t state = (uint32_t)((createLongMask(bits, bitsPerEntry) & ourLong) >> bits);
                            //sanity check 2
                            if((localPalette != NULL && state > paletteLength) || state > version->blocks.sz){
                                errno = E2BIG; //FIXME: this also happens. No clue why. The mask is generated fine, the bitshift is fine, the and works fine, it's the int64 that is wrong
                                return -2;
                            }
                            states[index] = state;
                            index++;
                        }
                    }
                    //and now foreach block
                    forBlocks(){    
                        //initialize the new block
                        block* newBlock = malloc(sizeof(block));
                        newBlock->entity = NULL;
                        newBlock->animationData = 0;
                        newBlock->stage = 0;
                        //get the state
                        int id = states[statesFormula(x, y, z)];
                        if(localPalette != NULL){ //apply the localPalette if necessary
                            id = localPalette[id];
                        }
                        //and then apply the global palette and boom type gotten
                        newBlock->type = version->blocks.palette[id];
                        newBlock->x = x;
                        newBlock->y = y;
                        newBlock->z = z;
                        s.blocks[x][y][z] = newBlock;
                    }
                    free(localPalette);
                    //TODO: handle biomes
                }
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
        case PLAYER_ABILITIES:{
            output->player.flags = readByte(input->data, &offset);
            output->player.flyingSpeed = readBigEndianFloat(input->data, &offset);
            output->player.fovModifier = readBigEndianFloat(input->data, &offset);
            break;
        }
        case PLAYER_INFO_UPDATE:{
            //this variable defines what info is appended to each player info element 
            byte actions = readByte(input->data, &offset);
            output->playerInfo.number = readVarInt(input->data, &offset);
            for(int i = 0; i < output->playerInfo.number; i++){
                struct genericPlayer* new = malloc(sizeof(struct genericPlayer));
                memset(new, 0, sizeof(struct genericPlayer));
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
                addElement(output->playerInfo.players, new);
            }
            break;
        }
        case SYNCHRONIZE_PLAYER_POSITION:{
            //this is the non responsive version of the handler
            handleSynchronizePlayerPosition(input, output, &offset);
            break;
        }
        case UPDATE_RECIPE_BOOK:{
            //MAYBE:
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
            foreachListElement(output->chunks){
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
            entity* our = getEntity(output->entityList, eid);
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
        case FEATURE_FLAGS:{
            int32_t count = readVarInt(input->data, &offset);
            output->featureFlags.flags = calloc(count, sizeof(identifier));
            for(int i = 0; i < count; i++){
                output->featureFlags.flags[i] = readString(input->data, &offset);
            }
            output->featureFlags.count = count;
            break;
        }
        case UPDATE_RECIPES:{
            //MAYBE: is this necessary?
            break;
        }
        case UPDATE_TAGS:{
            //int32_t count = readVarInt(input->data, &offset);
            //MAYBE:, what is this for?
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
    g.blockEntities = initList();
    g.playerInfo.players = initList();
    return g;
}

void freeGamestate(struct gamestate* g){
    freeList(g->blockEntities, (void(*)(void*))freeBlockEntity);
    freeList(g->entityList, free);
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
    freeList(g->playerInfo.players, (void(*)(void*))freeGenericPlayer);
    nbt_free(g->registryCodec);
    free(g->serverData.icon.bytes);
    free(g->serverData.MOTD);
}

static int getEntityId(const cJSON* entities, const char* name){
    cJSON* node = cJSON_GetObjectItemCaseSensitive(entities, name);
    node = cJSON_GetObjectItemCaseSensitive(node, "id");
    return node->valueint;
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
                        if(c->sections[s].blocks[x][y][z] != NULL){
                            free(c->sections[s].blocks[x][y][z]->type);
                            nbt_free(c->sections[s].blocks[x][y][z]->entity->tag);
                            free(c->sections[s].blocks[x][y][z]->entity);
                            free(c->sections[s].blocks[x][y][z]);
                        }
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

entity* getEntity(listHead* entities, int32_t eid){
    listEl* el = entities->first;
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
            //TODO: figure out how does a particle look like
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

struct gameVersion* createVersionStruct(const char* versionJSON){
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
    const cJSON* version = cJSON_ParseWithLength(jsonContents, sz);
    free(jsonContents);
    if(version == NULL){
        return NULL;
    }
    struct gameVersion* thisVersion = malloc(sizeof(struct gameVersion));
    //then we get the entities list (I could do with a hashmap here)
    thisVersion->entities = cJSON_GetObjectItemCaseSensitive(version, "entities");
    if(thisVersion->entities == NULL){
        return NULL;
    }
    //then we create a lookup table
    {
        const cJSON* blocks = cJSON_GetObjectItemCaseSensitive(version, "blocks");
        thisVersion->blocks.sz = cJSON_GetArraySize(blocks);
        thisVersion->blocks.palette = calloc(thisVersion->blocks.sz, sizeof(identifier));
        cJSON* child = blocks->child;
        for(int i = 0; i < thisVersion->blocks.sz; i++){
            cJSON* id = cJSON_GetObjectItemCaseSensitive(child, "id");
            thisVersion->blocks.palette[id->valueint] = child->string;
            child = child->next;
        }
    }
    thisVersion->json = version;
    return thisVersion;
}

void freeVersionStruct(struct gameVersion* version){
    cJSON_Delete((cJSON*)version->json);
    free(version->blocks.palette);
    free(version);
}

void freeBlockEntity(blockEntity* e){
    nbt_free(e->tag);
    free(e);
}

void freeProperty(struct property* p){
    free(p->name);
    free(p->signature);
    free(p->value);
    free(p);
}

void freeGenericPlayer(struct genericPlayer* p){
    free(p->displayName);
    free(p->name);
    freeList(p->properties, (void(*)(void*))freeProperty);
    free(p);
}
