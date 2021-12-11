#pragma once
#define RS_ADDRESS_STRING_SIZE 18
#define RS_ADDRESS_BYTE_SIZE 17
#include "globalTypes.h"

/** Transforms an account ID into a byte array suitable for
 * comparision with a mask */
void idToByteAccount(uint64_t accountId, uint8_t * out);

/** Transforms an account ID in a Reed Solomon address */
void idToAccount(uint64_t accountId, char * out);

/** Parse a string with a suposed mask and leave the result
 * inside byte mask */
void maskToByteMask(const char * charMask, uint8_t * byteMask, int32_t isSuffix);

/** Compares a byte mask with a byte account.
 * Return 1 if it matches, otherwise returns 0 */
uint8_t matchMask(const uint8_t * byteMask, const uint8_t * account);
