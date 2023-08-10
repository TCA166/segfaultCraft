
typedef unsigned char byte;

//Type for Minecraft UUID which should be unsigned int128
typedef __uint128_t UUID_t;

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

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

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
 @param index the index at which the value should be read, is incremented by the number of bytes the VarInt took
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
 @brief Parses an encoded string
 @param buff the buffer within which the string is encoded
 @param index the index at which the string is encoded, is incremented by the number of bytes the string and VarInt took
 @return the pointer to the encoded string 
*/
char* readString(const byte* buff, int* index);
