#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"
#include "cJSON/cJSON.h"

static int getEntityId(const cJSON* entities, const char* name);

static block** getBlock(struct gamestate* current, position pos);

#define event(gamestate, name, ...) \
    int result = (*gamestate->eventHandlers.name)(__VA_ARGS__); \
    if(result < 0){ \
        return result; \
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

int parsePlayPacket(packet* input, struct gamestate* output, const cJSON* entities, const identifier* globalPalette){
    int offset = 0;
    switch(input->packetId){
        case SPAWN_ENTITY:{
            entity* e = malloc(sizeof(entity));
            e->id = readVarInt(input->data, &offset);
            e->uid = readUUID(input->data, &offset);
            e->type = readVarInt(input->data, &offset);
            e->x = readDouble(input->data, &offset);
            e->y = readDouble(input->data, &offset);
            e->z = readDouble(input->data, &offset);
            e->pitch = readByte(input->data, &offset);
            e->yaw = readByte(input->data, &offset);
            e->headYaw = readByte(input->data, &offset);
            e->data = readVarInt(input->data, &offset);
            e->velocityX = readShort(input->data, &offset);
            e->velocityY = readShort(input->data, &offset);
            e->velocityZ = readShort(input->data, &offset);
            addElement(output->entityList, (void*)e);
            event(output, spawnEntityHandler, e)
            break;
        }
        case SPAWN_EXPERIENCE_ORB:{
            entity* exp = malloc(sizeof(entity));
            exp->id = readVarInt(input->data, &offset);
            exp->type = getEntityId(entities, "minecraft:experience_orb");
            exp->x = readDouble(input->data, &offset);
            exp->y = readDouble(input->data, &offset);
            exp->z = readDouble(input->data, &offset);
            exp->data = (int32_t)readShort(input->data, &offset);
            addElement(output->entityList, (void*)exp);
            event(output, spawnEntityHandler, exp)
            break;
        }
        case SPAWN_PLAYER:{
            entity* player = malloc(sizeof(entity));
            player->id = readVarInt(input->data, &offset);
            player->uid = readUUID(input->data, &offset);
            player->type = getEntityId(entities, "minecraft:player");
            player->x = readDouble(input->data, &offset);
            player->y = readDouble(input->data, &offset);
            player->z = readDouble(input->data, &offset);
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
            //TODO: finish
            break;
        }
        case ACKNOWLEDGE_BLOCK_CHANGE:{
            int32_t sequenceChange = readVarInt(input->data, &offset);
            for(int i = 0; i < output->pendingChanges.len; i++){
                struct blockChange* change = output->pendingChanges.array + i;
                if(change->sequenceId == sequenceChange){
                    //TODO handle rest
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
            int32_t breakingId = readVarInt(input->data, &offset);
            position location = (position)readLong(input->data, &offset);
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
        case BLOCK_ENTITY_DATA:;
            //TODO: HANDLE
            break;
        case BLOCK_ACTION:{
            position location = (position)readLong(input->data, &offset);
            uint16_t animationData = readShort(input->data, &offset);
            block** b = getBlock(output, location);
            if(b != NULL){
                (*b)->animationData = animationData;
                event(output, blockActionHandler, *b)
            }
            break;
        }
        case BLOCK_UPDATE:{
            position location = (position)readLong(input->data, &offset);
            int32_t blockId = readVarInt(input->data, &offset);
            block** b = getBlock(output, location);
            if(b != NULL){
                (*b)->type = globalPalette[blockId];
            }
            break;
        }
        case BOSS_BAR:{
            //TODO finish this
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
                int32_t chunkX = readInt(input->data, &offset);
                int32_t chunkZ = readInt(input->data, &offset);
                byteArray chunk = readByteArray(input->data, &offset);
                //TODO finish
                free(chunk.bytes);
            }
            break;
        }
        case CLEAR_TITLES:{
            //TODO finish and figure out what is this for
            break;
        }
        case COMMAND_SUGGESTIONS_RESPONSE:{
            //TODO handle
            break;
        }
        case COMMANDS:{
            //TODO handle
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
            int32_t stateId = readVarInt(input->data, &offset);
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
            int16_t property = readShort(input->data, &offset);
            int16_t value = readShort(input->data, &offset);
            if(output->openContainer != NULL && output->openContainer->id == windowId){
                output->openContainer->flags[property] = value;
                event(output, containerHandler, output->openContainer);
            }
            break;
        }
        case SET_CONTAINER_SLOT:{
            byte windowId = readByte(input->data, &offset);
            int32_t stateId = readVarInt(input->data, &offset);
            int16_t slotId = readShort(input->data, &offset);
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
            //TODO handle
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
                sourceX = readDouble(input->data, &offset);
                sourceY = readDouble(input->data, &offset);
                sourceZ = readDouble(input->data, &offset);
            }
            event(output, damageHandler, entityId, sourceType, sourceCause, sourceDirect, hasPosition, sourceX, sourceY, sourceZ)
            break;
        }
        case DELETE_MESSAGE:{
            //TODO implement alongside the rest of the chat packets
            break;
        }
        case DISGUISED_CHAT_MESSAGE:{
            //TODO
            break;
        }
        case ENTITY_EVENT:{
            int32_t id = readInt(input->data, &offset);
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
            double X = readDouble(input->data, &offset);
            double Y = readDouble(input->data, &offset);
            double Z = readDouble(input->data, &offset);
            float strength = readFloat(input->data, &offset);
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
            output->player.X += readFloat(input->data, &offset);
            output->player.Y += readFloat(input->data, &offset);
            output->player.Z += readFloat(input->data, &offset);
            event(output, explosionHandler, X, Y, Z, strength)
            break;
        }
        case UNLOAD_CHUNK:{
            int32_t X = readInt(input->data, &offset);
            int32_t Z = readInt(input->data, &offset);
            listEl* el = output->chunks->first;
            while(el != NULL){
                chunk* c = el->value;
                if(c != NULL && c->x == X && c->z == Z){
                    freeChunk(c);
                    unlinkElement(el);
                    freeListElement(el, false);
                }
                el = el->next;
            }
            break;
        }
        case GAME_EVENT:{
            byte eventId = readByte(input->data, &offset);
            float value = readFloat(input->data, &offset);
            if(output->eventHandlers.generic[eventId] != NULL){
                event(output, generic[eventId], value)
            }
            break;
        }
        case OPEN_HORSE_SCREEN:{
            byte windowId = readByte(input->data, &offset);
            int32_t slotCount = readVarInt(input->data, &offset);
            int32_t eid = readInt(input->data, &offset);
            entity* horse = getEntity(output->entityList, eid);
            if(horse == NULL){
                break;
            }
            const cJSON* ent = entities;
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
            output->openContainer->type = 1; //TODO change to actual value
            event(output, containerHandler, output->openContainer)
            break;
        }
        case HURT_ANIMATION:{
            int32_t eid = readVarInt(input->data, &offset);
            float yaw = readFloat(input->data, &offset);
            event(output, hurtAnimationHandler, getEntity(output->entityList, eid), yaw)
            break;
        }
        case INITIALIZE_WORLD_BORDER:{
            output->worldBorder.X = readDouble(input->data, &offset);
            output->worldBorder.Z = readDouble(input->data, &offset);
            double oldDiameter = readDouble(input->data, &offset);
            output->worldBorder.diameter = readDouble(input->data, &offset);
            output->worldBorder.speed = readVarLong(input->data, &offset);
            output->worldBorder.portalBoundary = readVarInt(input->data, &offset);
            output->worldBorder.warning = readVarInt(input->data, &offset);
            output->worldBorder.warningT = readVarInt(input->data, &offset);
            event(output, worldBorder, oldDiameter)
            break;
        }
        case CHUNK_DATA_AND_UPDATE_LIGHT:{
            //TODO URGENT
            break;
        }
        case LOGIN_PLAY:{
            output->loginPlay = true;
            output->player.entityId = readInt(input->data, &offset);
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
            output->hashedSeed = readLong(input->data, &offset);
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
                output->death = readLong(input->data, &offset);
            }
            output->portalCooldown = readVarInt(input->data, &offset);
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
    g.entityList = initList();
    g.chunks = initList();
    return g;
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
    short sectionId = (y + (4 * 16)) >> 4;
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
    return &(c->sections[sectionId].blocks[x][y][z]);
}

void freeChunk(chunk* c){
    for(uint8_t s = 0; s < 24; s++){
        for(uint8_t x = 0; x < 16; x++){
            for(uint8_t y = 0; y < 16; y++){
                for(uint8_t z = 0; z < 16; z++){
                    free(c->sections[s].blocks[x][y][z]->type);
                    free(c->sections[s].blocks[x][y][z]);
                }
            }
        }
    }
    free(c);
}