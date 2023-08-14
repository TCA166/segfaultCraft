#include <stdlib.h>
#include <string.h>
#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"

size_t nodeSize(nbt_node* node){
    size_t res = 1;
    if(node->name != NULL){
        res += 2 + strlen(node->name);
    }
    struct list_head* pos;
    switch(node->type){
        case TAG_INVALID:;
            res += 1;
            break;
        case TAG_BYTE:;
            res += 1;
            break;
        case TAG_SHORT:;
            res += 2;
            break;
        case TAG_INT:;
            res += 4;
            break;
        case TAG_LONG:;
            res += 8;
            break;
        case TAG_FLOAT:;
            res += 4;
            break;
        case TAG_DOUBLE:;
            res += 8;
            break;
        case TAG_BYTE_ARRAY:;
            res += 4 + node->payload.tag_byte_array.length;
            break;
        case TAG_STRING:;
            res += 2 + strlen(node->payload.tag_string);
            break;
        case TAG_LIST:;
            res += 5;
            list_for_each(pos, &node->payload.tag_list->entry){
                struct nbt_list* el = list_entry(pos, struct nbt_list, entry);
                res += nodeSize(el->data);
            }
            break;
        case TAG_COMPOUND:;
            list_for_each(pos, &node->payload.tag_list->entry){
                struct nbt_list* el = list_entry(pos, struct nbt_list, entry);
                res += nodeSize(el->data);
            }
            break;
        case TAG_INT_ARRAY:;
            res += 4 + (node->payload.tag_int_array.length * 4);
            break;
        case TAG_LONG_ARRAY:;
            res += 4 + (node->payload.tag_long_array.length * 8);
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
            stringArray dimemsionNames = readStringArray(input->data, &offset);
            output->dimensions.len = dimemsionNames.len;
            output->dimensions.arr = (identifier*)dimemsionNames.arr;
            nbt_node* registryCodec = nbt_parse(input->data + offset, input->size - offset);
            if(registryCodec == NULL || registryCodec->type != TAG_COMPOUND){
                return -1;
            }
            output->registryCodec = registryCodec;
            offset += nodeSize(registryCodec);
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