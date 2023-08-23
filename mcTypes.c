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

//Gets the size of the nbt tag in buffer, assumes the nbt tag is of the given type
static inline size_t tagSize(const byte* buff, byte type);

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

int64_t readLong(const byte* buff, int* index){
    getIndex(index);
    int64_t result = *(int64_t*)(buff + *index);
    *index += sizeof(int64_t);
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

int16_t readShort(const byte* buff, int* index){
    getIndex(index)
    int16_t result = *(int16_t*)(buff + *index);
    *index += sizeof(int16_t);
    return result;
}

int16_t readBigEndianShort(const byte* buff, int* index){
    int16_t s = readShort(buff, index);
    return (s << 8) | ((s >> 8) & 0xFF);
}

int32_t readBigEndianInt(const byte* buff, int* index){
    int32_t i = readInt(buff, index);
    i = ((i << 8) & 0xFF00FF00) | ((i >> 8) & 0xFF00FF ); 
    return (i << 16) | ((i >> 16) & 0xFFFF);
}

UUID_t readUUID(const byte* buff, int* index){
    getIndex(index)
    UUID_t result = *(UUID_t*)(buff + *index);
    *index += sizeof(UUID_t);
    return result;
}

double readDouble(const byte* buff, int* index){
    getIndex(index)
    double result = *(double*)(buff + *index);
    *index += sizeof(double);
    return result;
}

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

slot readSlot(const byte* buff, int* index){
    getIndex(index)
    slot result = {};
    result.present = readBool(buff, index);
    if(result.present){
        result.id = readVarInt(buff, index);
        result.count = readByte(buff, index);
        size_t sz = nbtSize(buff + *index, false);
        result.NBT = nbt_parse(buff + *index, sz);
        *index += sz;
    }
    return result;
}

float readFloat(const byte* buff, int* index){
    getIndex(index)
    float result = *(float*)buff;
    *index += sizeof(float);
    return result;
}

int64_t readVarLong(const byte* buff, int* index){
    int64_t value = 0;
    int position = 0;
    getIndex(index)
    while(true){
        byte currentByte = buff[*index];
        *index += 1;
        value |= (int64_t)(currentByte & SEGMENT_BITS) << position;

        if((currentByte & CONTINUE_BIT) == 0) break;

        position += 7;

        if(position >= 64){
            errno = EOVERFLOW;
            return 0;
        }
    }
    return value;
}
