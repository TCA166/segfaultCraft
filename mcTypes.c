#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "mcTypes.h"

//If index is null then changes index to point to a newly created int with value of 0
#define getIndex(index) \
    if(index == NULL){ \
        int locIndex = 0; \
        index = &locIndex; \
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
    getIndex(index)
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
    getIndex(index)
    int msgLen = readVarInt(buff, index);
    char* result = calloc(msgLen + 1, sizeof(char));
    memcpy(result, buff + *index, msgLen);
    *index += msgLen;
    return result;
}

size_t writeByteArray(byte* buff, byteArray arr){
    size_t offset = writeVarInt(buff, arr.len);
    memcpy(buff + offset, arr.bytes, arr.len);
    return offset + arr.len;
}

byteArray readByteArray(const byte* buff, int* index){
    getIndex(index)
    byteArray arr = nullByteArray;
    int arrLen = readVarInt(buff, index);
    arr.len = arrLen;
    byte* val = calloc(arrLen, sizeof(byte));
    memcpy(val, buff + *index, arrLen);
    arr.bytes = val;
    *index += arrLen;
    return arr;
}

int32_t readInt(const byte* buff, int* index){
    getIndex(index)
    int32_t result = *(int32_t*)(buff + *index);
    *index += sizeof(int32_t);
    return result;
}

bool readBool(const byte* buff, int* index){
    getIndex(index)
    bool result =  *(bool*)(buff + *index);
    *index += sizeof(bool);
    return result;
}

byte readByte(const byte* buff, int* index){
    getIndex(index)
    byte result = buff[*index];
    *index += 1;
    return result;
}

stringArray readStringArray(const byte* buff, int* index){
    stringArray result = nullStringArray;
    getIndex(index)
    int number = readVarInt(buff, index);
    result.len = number;
    result.arr = calloc(number, sizeof(char*));
    for(int i = 0; i < number; i++){
        result.arr[i] = readString(buff, index);
    }
    return result;
}
