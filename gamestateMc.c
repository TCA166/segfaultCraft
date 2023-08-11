#include <stdlib.h>
#include "gamestateMc.h"
#include "packetDefinitions.h"
#include "stdbool.h"
#include "cNBT/nbt.h"

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
            output->registryCodec = registryCodec; //HUH NOW HOW DO I GET THE NUMBER OF BYTES READ
            //size_t s = nbt_size(registryCodec);
            //fprintf(stderr, "%d", s);
            break;  
        default:;
            return -1;
            break;
    }
    return 0;
}