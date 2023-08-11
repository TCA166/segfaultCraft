#include "stdbool.h"

typedef unsigned char byte;

//Type for Minecraft UUID which should be unsigned int128
typedef __uint128_t UUID_t;

//Minecraft identifier - a string in this format space:name
typedef char* identifier;

typedef int64_t position;

#define positionX(position) (position >> 38)

#define positionY(position) (position & 0xfff)

#define positionZ(position) (position & << 26 >> 38)

/*!
 @struct byteArray
 @brief An array with an attached size_t
 @param bytes the array itself
 @param len the length of the bytes array
*/
typedef struct byteArray{
    byte* bytes;
    size_t len;
} byteArray;

#define nullByteArray (byteArray){NULL, 0}

typedef struct stringArray{
    char** arr;
    size_t len;
} stringArray;

typedef struct identifierArray{
    identifier* arr;
    size_t len;
} identifierArray;

#define nullStringArray (stringArray){NULL, 0}

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

#define MAX_VAR_INT 4
#define MAX_VAR_LONG 8

/*! 
 @brief Writes the given value to the buffer as VarInt
 @param buff pointer to an allocated buffer where the VarInt bytes shall be written to
 @param value the value that will be written to the buffer
 @return the number of bytes written to buff
*/
size_t writeVarInt(byte* buff, int32_t value);

/*! 
 @brief Reads the VarInt from buffer at the given index
 @param buff the buffer from which to read the value
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the int encoded as VarInt in buff at index
*/
int32_t readVarInt(const byte* buff, int* index);

/*! 
 @brief Writes the given string to the buffer with a VarInt length preceding it
 @param buff the buffer that the string will be written to
 @param string the string that is to be written to buff
 @param stringLen the length of string
 @return the number of bytes written, including the size of VarInt in front of the string
*/
size_t writeString(byte* buff, const char* string, int stringLen);

/*!
 @brief Parses an encoded Minecraft string and writes it into a newly allocated buffer making it NULL terminated
 @param buff the buffer within which the string is encoded
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the pointer to the encoded string 
*/
char* readString(const byte* buff, int* index);

/*!
 @brief Writes a byteArray to the buffer
 @param buff the buffer to which the byteArray should be written to
 @param arr the byteArray that should be written
 @return the number of bytes written 
*/
size_t writeByteArray(byte* buff, byteArray arr);

/*!
 @brief Parses an encoded Minecraft byte array and writes it into a newly allocated buffer
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return a byte array
*/
byteArray readByteArray(const byte* buff, int* index);

/*!
 @brief Reads an int32 from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded int32
*/
int32_t readInt(const byte* buff, int* index);

/*!
 @brief Read a boolean from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the encoded boolean
*/
bool readBool(const byte* buff, int* index);

/*!
 @brief Reads a byte from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return the byte
*/
byte readByte(const byte* buff, int* index);

/*!
 @brief Reads an array of strings from the buffer at index
 @param buff the buffer to read from
 @param index the pointer to the index at which the value should be read, is incremented by the number of bytes read. Can be NULL, at which point index=0
 @return a stringArray object
*/
stringArray readStringArray(const byte* buff, int* index);