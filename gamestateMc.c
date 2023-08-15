#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"

//Gets the size of the nbt tag in buffer, assumes the nbt tag is of the given type
static size_t tagSize(const byte* buff, byte type);

//Gets the size of the compound tag and it's children
static size_t compoundSize(const byte* buff);

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

size_t tagSize(const byte* buff, byte type){
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
            res += compoundSize(buff + res);
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

size_t compoundSize(const byte* buff){
    int res = 0;
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
    //fprintf(stderr, "%d\n", res);
    return res;
}

//Nbt calculating function using cNBT nodes instead of memory buffers
size_t nodeSize(nbt_node* node, bool compound){
    size_t res = sizeof(byte);
    if(compound || res == TAG_COMPOUND){   
        res += sizeof(int16_t);
        if(node->name != NULL){
            res += strlen(node->name);
        }
    }
    struct list_head* pos;
    switch(node->type){
        case TAG_INVALID:;
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
            res += sizeof(int32_t) + node->payload.tag_byte_array.length;
            break;
        case TAG_STRING:;
            res += sizeof(int16_t) + strlen(node->payload.tag_string);
            break;
        case TAG_LIST:;
            res += sizeof(byte) + sizeof(int32_t);
            int n = 0;
            list_for_each(pos, &node->payload.tag_list->entry){
                struct nbt_list* el = list_entry(pos, struct nbt_list, entry);
                res += nodeSize(el->data, false);
                n++;
            }
            if(n <= 0){
                res += 1;
            }
            break;
        case TAG_COMPOUND:;
            list_for_each(pos, &node->payload.tag_list->entry){
                struct nbt_list* el = list_entry(pos, struct nbt_list, entry);
                res += nodeSize(el->data, true);
            }
            break;
        case TAG_INT_ARRAY:;
            res += sizeof(int32_t) + (node->payload.tag_int_array.length * sizeof(int32_t));
            break;
        case TAG_LONG_ARRAY:;
            res += sizeof(int32_t) + (node->payload.tag_long_array.length * sizeof(int64_t));
            break;
    }
    return res;
}

int parsePlayPacket(packet* input, struct gamestate* output){
    int offset = 0;
    switch(input->packetId){
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
                size_t sz1 = nbtSize(nbt, false);
                nbt_node* registryCodec = nbt_parse(nbt, sz1);
                if(registryCodec == NULL || registryCodec->type != TAG_COMPOUND){
                    return -1;
                }
                output->registryCodec = registryCodec;
                //size_t sz = nodeSize(registryCodec, false); //sz is off the actual size by about 263 bytes, so it should be 32752
                offset += sz1; //this one is now too big 
            }
            byte* pastNbt = input->data + offset;
            output->dimensionType = readString(input->data, &offset);            
            //size_t s = nbt_size(registryCodec);
            //fprintf(stderr, "%d", s);
            break;  
        default:;
            return -1;
            break;
    }
    return 0;
}