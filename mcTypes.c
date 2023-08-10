#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "mcTypes.h"

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

size_t writeString(byte* buff, const char* string, int stringLen){
    for(int i = stringLen - 1; i >= 0; i--){
        if(string[i] == '\0'){
            stringLen--;
        }
    }
    size_t offset = writeVarInt(buff, stringLen);
    memcpy(buff + offset, string, stringLen);
    return stringLen + offset;
}

char* readString(const byte* buff, int* index){
    if(index == NULL){
        int locIndex = 0;
        index = &locIndex;
    }
    int msgLen = readVarInt(buff, index);
    char* result = calloc(msgLen + 1, sizeof(char));
    memcpy(result, buff + *index, msgLen);
    return result;
}

size_t writeByteArray(byte* buff, byteArray arr){
    size_t offset = writeVarInt(buff, arr.len);
    memcpy(buff + offset, arr.bytes, arr.len);
    return offset + arr.len;
}

byteArray readByteArray(const byte* buff, int* index){
    if(index == NULL){
        int locIndex = 0;
        index = &locIndex;
    }
    byteArray arr = nullByteArray;
    int arrLen = readVarInt(buff, index);
    arr.len = arrLen;
    byte* val = calloc(arrLen, sizeof(byte));
    memcpy(val, buff + *index, arrLen);
    arr.bytes = val;
    return arr;
}
