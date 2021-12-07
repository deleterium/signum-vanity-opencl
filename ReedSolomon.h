#define RS_ADDRESS_STRING_SIZE 18
#define RS_ADDRESS_BYTE_SIZE 17

typedef unsigned char BYTE;

/** Transforms an account ID into a byte array suitable for
 * comparision with a mask */
void idTOByteAccount(unsigned long accountId, BYTE * out);

/** Transforms an account ID in a Reed Solomon address */
void idTOAccount(unsigned long accountId, char * out);

/** Parse a string with a suposed mask and leave the result
 * inside byte mask */
void maskToByteMask(const char * charMask, BYTE * byteMask);

/** Compares a byte mask with a byte account.
 * Return 1 if it matches, otherwise returns 0 */
unsigned char matchMask(const BYTE * byteMask, const BYTE * account);
