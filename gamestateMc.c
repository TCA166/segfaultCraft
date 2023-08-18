#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"

//Gets the size of the nbt tag in buffer, assumes the nbt tag is of the given type
static inline size_t tagSize(const byte* buff, byte type);

//Gets the size of the nbt tag and it's children
size_t nbtSize(const byte* buff, bool inCompound);

size_t nbtSize(const byte* buff, bool inCompound){
    int res = 0;
    //each nbt tag starts with a byte indicating the type
    byte type = readByte(buff, &res);
    if(type > TAG_LONG_ARRAY || (type == TAG_INVALID && !inCompound)){
        return 0; //something went very wrong
    }
    //and then tags other than TAG_END if in compound tag have always 2 bytes indicating name length
    if((inCompound || type == TAG_COMPOUND) && type != TAG_INVALID){
        res += readBigEndianShort(buff, &res); //by doing this trick we increment res twice. Once by passing the pointer(by short size) and once by incrementing by returned value
    }
    //Then we just continue parsing
    res += tagSize(buff + res, type);
    return res;
}

static inline size_t tagSize(const byte* buff, byte type){
    int res = 0;
    switch(type){
        case TAG_INVALID:;
            return 0; //we return 0 to indicate INVALID
            break;
        case TAG_BYTE:;
            res += sizeof(byte);
            break;
        case TAG_SHORT:;
            res += sizeof(int16_t);
            break;
        case TAG_INT:;
            res += sizeof(int32_t);
            break;
        case TAG_LONG:;
            res += sizeof(int64_t);
            break;
        case TAG_FLOAT:;
            res += sizeof(float);
            break;
        case TAG_DOUBLE:;
            res += sizeof(double);
            break;
        case TAG_BYTE_ARRAY:;
            res += readBigEndianInt(buff, &res);
            break;
        case TAG_STRING:;
            res += (uint16_t)readBigEndianShort(buff, &res);
            break;
        case TAG_LIST:;
            //each list element has the same type
            byte type = readByte(buff, &res);
            int32_t num = readBigEndianInt(buff, &res);
            if(num <= 0){ //if the list is empty we need to increment the size
                res += 1; //for whatever reason empty lists AREN'T FULLY EMPTY
                //AND THE WORST PART IS I CANNOT DETECT REALISTICALLY HOW MUCH EMPTY BYTES ARE APPENDED 
            }
            else{
                //else we parse each element in list (why are arrays named lists here?)
                while(num > 0){
                    res += tagSize(buff + res, type);
                    num--;
                }
            }
            break;
        case TAG_COMPOUND:;
            while(true){
                const byte* el = buff + res;
                size_t r = nbtSize(el, true);
                if(r == 0){
                    break;
                }
                else{
                    //fprintf(stderr, "%d ", r);
                    res += r;
                }
            }
            break;
        case TAG_INT_ARRAY:;
            res += readBigEndianInt(buff, &res) * sizeof(int32_t);
            break;
        case TAG_LONG_ARRAY:;
            res += readBigEndianInt(buff, &res) * sizeof(int64_t);
            break;
    }
    return res;
}

int parsePlayPacket(packet* input, struct gamestate* output){
    int offset = 0;
    switch(input->packetId){
        case SPAWN_ENTITY:;
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
            break;
        case LOGIN_PLAY:;
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
        case CHUNK_BIOMES:;
            int num = readVarInt(input->data, &offset);
            while(num > 0){
                int32_t chunkX = readInt(input->data, &offset);
                int32_t chunkZ = readInt(input->data, &offset);
                byteArray chunk = readByteArray(input->data, &offset);
                free(chunk.bytes);
            }
            break;
        default:;
            return -1;
            break;
    }
    return 0;
}

struct gamestate initGamestate(){
    struct gamestate g = {};
    g.entityList = initList();
    return g;
}