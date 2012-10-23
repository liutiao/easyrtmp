#ifndef G711_H
#define G711_H

/**
Encode a buffer of 16 bit uniform PCM values into A-Law values
@param dst	   Pointer to location to store A-Law encoded values
@param src	   Pointer to the buffer of 16 bit uniform PCM values to be encoded
@param srcSize The size, in bytes, of the buffer at \a src
@return 	   The number of bytes which were stored at dst (equal to srcSize>>1)
*/
extern unsigned ALawEncode(unsigned char* dst, short* src, size_t srcSize);

/**
Decode a buffer of A-Law values into 16 bit uniform PCM values
@param dst	   Pointer to location to store decoded 16 bit uniform PCM values
@param src	   Pointer to the buffer of A-Law values to be decoded
@param srcSize The size, in bytes, of the buffer at \a src
@return 	   The number of bytes which were stored at \a dst (equal to srcSize<<1)
*/
extern unsigned ALawDecode(short* dst, const unsigned char* src, size_t srcSize);

/**
Decode a buffer of u-Law values into 16 bit uniform PCM values
@param dst	   Pointer to location to store decoded 16 bit uniform PCM values
@param src	   Pointer to the buffer of u-Law values to be decoded
@param srcSize The size, in bytes, of the buffer at \a src
@return 	   The number of bytes which were stored at \a dst (equal to srcSize<<1)
*/
unsigned ULawDecode(short* dst, const unsigned char* src, size_t srcSize);

/**
Encode a buffer of 16 bit uniform PCM values into u-Law values
@param dst	   Pointer to location to store u-Law encoded values
@param src	   Pointer to the buffer of 16 bit uniform PCM values to be encoded
@param srcSize The size, in bytes, of the buffer at \a src
@return 	   The number of bytes which were stored at \a dst (equal to srcSize>>1)
*/
unsigned ULawEncode(unsigned char* dst, short* src, size_t srcSize);

/**
Convert a buffer of A-Law values into u-law values.
@param dst	   Pointer to location to store u-law values
@param src	   Pointer to the buffer of A-Law values to be converted
@param srcSize The size, in bytes, of the buffer at \a src
@return 	   The number of bytes which were stored at \a dst (equal to srcSize)
*/
unsigned ALawToULaw(unsigned char* dst, const unsigned char* src, size_t srcSize);

/**
Convert a buffer of u-Law values into A-law values.
@param dst	   Pointer to location to store A-law values
@param src	   Pointer to the buffer of u-Law values to be converted
@param srcSize The size, in bytes, of the buffer at \a src
@return 	   The number of bytes which were stored at \a dst (equal to srcSize)
*/
unsigned ULawToALaw(unsigned char* dst, const unsigned char* src, size_t srcSize);

#endif

