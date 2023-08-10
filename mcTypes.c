#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "mcTypes.h"

size_t writeString(byte* buff, const char* string, int stringLen){
    size_t offset = writeVarInt(buff, stringLen);
    strcpy(buff + offset, string);
    return stringLen + offset;
}

size_t writeVarInt(byte* buff, int32_t value){
    short i = 0;
    while(true){
        if((value & ~SEGMENT_BITS) == 0){
            buff[i] = value;
            return i + 1;
        }
        buff[i] = (value & SEGMENT_BITS) | CONTINUE_BIT;
        value >>= 7;
        i++;
    }
}

int32_t readVarInt(const byte* buff, int* index){
    int32_t value = 0;
    int position = 0;
    if(index == NULL){
        int locIndex = 0;
        index = &locIndex;
    }
    while(true){
        byte currentByte = buff[*index];
        *index += 1;
        value |= (currentByte & SEGMENT_BITS) << position;

        if((currentByte & CONTINUE_BIT) == 0) break;

        position += 7;

        if(position >= 32){
            errno = EOVERFLOW;
            return 0;
        }
    }
    return value;
}

char* readString(const byte* buff, int* index){
    if(index == NULL){
        int locIndex = 0;
        index = &locIndex;
    }
    int msgLen = readVarInt(buff, index);
    return (char*)(buff + *index);
}
