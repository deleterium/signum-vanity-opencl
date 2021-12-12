#pragma once
#define RS_ADDRESS_STRING_SIZE 21
#define RS_ADDRESS_BYTE_SIZE 17
#include "globalTypes.h"

#define MASK_MATCH_2 0
#define MASK_MATCH_3 1
#define MASK_MATCH_4 2
#define MASK_MATCH_5 3
#define MASK_MATCH_6 4
#define MASK_MATCH_7 5
#define MASK_MATCH_8 6
#define MASK_MATCH_9 7
#define MASK_MATCH_A 8
#define MASK_MATCH_B 9
#define MASK_MATCH_C 10
#define MASK_MATCH_D 11
#define MASK_MATCH_E 12
#define MASK_MATCH_F 13
#define MASK_MATCH_G 14
#define MASK_MATCH_H 15
#define MASK_MATCH_J 16
#define MASK_MATCH_K 17
#define MASK_MATCH_L 18
#define MASK_MATCH_M 19
#define MASK_MATCH_N 20
#define MASK_MATCH_P 21
#define MASK_MATCH_Q 22
#define MASK_MATCH_R 23
#define MASK_MATCH_S 24
#define MASK_MATCH_T 25
#define MASK_MATCH_U 26
#define MASK_MATCH_V 27
#define MASK_MATCH_W 28
#define MASK_MATCH_X 29
#define MASK_MATCH_Y 30
#define MASK_MATCH_Z 31
#define MASK_MATCH_ANY 32
#define MASK_MATCH_ANY_NUMBER 33
#define MASK_MATCH_ANY_LETTER 34
#define MASK_MATCH_MINUS 35

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

/** Returns the chance of finding an address in one draw
 * given a byteMask */
double findingChance(uint8_t * byteMask);

/** From a given byteMask, return a printable user mask */
void byteMaskToPrintMask(uint8_t * byteMask, char * printMask);
