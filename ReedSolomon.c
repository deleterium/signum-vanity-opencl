#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ReedSolomon.h"

#define ginv(a) (gexp[31 - glog[a]])
#define BASE_32_LENGTH 13

const uint8_t gexp[] = {
    1, 2, 4, 8, 16, 5, 10, 20, 13, 26, 17, 7, 14, 28, 29, 31,
    27, 19, 3, 6, 12, 24, 21, 15, 30, 25, 23, 11, 22, 9, 18, 1
};
const uint8_t glog[] = {
    0, 0, 1, 18, 2, 5, 19, 11, 3, 29, 6, 27, 20, 8, 12, 23,
    4, 10, 30, 17, 7, 22, 28, 26, 21, 25, 9, 16, 13, 14, 24, 15
};
const uint8_t cwmap[] = {3, 2, 1, 0, 7, 6, 5, 4, 13, 14, 15, 16, 12, 8, 9, 10, 11};
const char rsAlphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
const char rsMaskAlphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ?#@";

uint8_t gmult(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) {
        return 0;
    }
    return gexp[(glog[a] + glog[b]) % 31];
}

void idToByteAccount(uint64_t accountId, uint8_t * out) {
    uint8_t codeword[RS_ADDRESS_BYTE_SIZE];
    uint8_t p[] = {0, 0, 0, 0};
    for (size_t i = 0; i < BASE_32_LENGTH; i++) {
        codeword[i] = accountId % 32;
        accountId >>= 5;
    }
    for (size_t i = BASE_32_LENGTH; i != 0; i--) {
        uint8_t fb = codeword[i - 1] ^ p[3];
        p[3] = p[2] ^ gmult(30, fb);
        p[2] = p[1] ^ gmult(6, fb);
        p[1] = p[0] ^ gmult(9, fb);
        p[0] = gmult(17, fb);
    }
    codeword[13] = p[0];
    codeword[14] = p[1];
    codeword[15] = p[2];
    codeword[16] = p[3];
    for (size_t i = 0; i < RS_ADDRESS_BYTE_SIZE; i++) {
        out[i] = codeword[cwmap[i]];
    }
}

void idToAccount(uint64_t accountId, char * out) {
    uint8_t byteOut[RS_ADDRESS_BYTE_SIZE];
    idToByteAccount(accountId, byteOut);
    for (size_t i = 0, j=0; i < RS_ADDRESS_BYTE_SIZE; i++, j++) {
        if (j==4 || j==9 || j==14) {
            out[j] = '-';
            j++;
        }
        out[j] = rsAlphabet[byteOut[i]];
    }
    out[RS_ADDRESS_STRING_SIZE - 1] = '\0';
}

uint8_t rscharToIndex(char in) {
    switch (in) {
    case '2': return MASK_MATCH_2;
    case '3': return MASK_MATCH_3;
    case '4': return MASK_MATCH_4;
    case '5': return MASK_MATCH_5;
    case '6': return MASK_MATCH_6;
    case '7': return MASK_MATCH_7;
    case '8': return MASK_MATCH_8;
    case '9': return MASK_MATCH_9;
    case 'A': return MASK_MATCH_A;
    case 'B': return MASK_MATCH_B;
    case 'C': return MASK_MATCH_C;
    case 'D': return MASK_MATCH_D;
    case 'E': return MASK_MATCH_E;
    case 'F': return MASK_MATCH_F;
    case 'G': return MASK_MATCH_G;
    case 'H': return MASK_MATCH_H;
    case 'J': return MASK_MATCH_J;
    case 'K': return MASK_MATCH_K;
    case 'L': return MASK_MATCH_L;
    case 'M': return MASK_MATCH_M;
    case 'N': return MASK_MATCH_N;
    case 'P': return MASK_MATCH_P;
    case 'Q': return MASK_MATCH_Q;
    case 'R': return MASK_MATCH_R;
    case 'S': return MASK_MATCH_S;
    case 'T': return MASK_MATCH_T;
    case 'U': return MASK_MATCH_U;
    case 'V': return MASK_MATCH_V;
    case 'W': return MASK_MATCH_W;
    case 'X': return MASK_MATCH_X;
    case 'Y': return MASK_MATCH_Y;
    case 'Z': return MASK_MATCH_Z;
    case '?': return MASK_MATCH_ANY;
    case '#': return MASK_MATCH_ANY_NUMBER;
    case '@': return MASK_MATCH_ANY_LETTER;
    case '-': return MASK_MATCH_MINUS;
    default:
        printf("Error parsing mask. Char '%c' is not allowed.  Try '--help'.\n", in);
        exit(1) ;
    }
}

void maskToByteMask(const char * charMask, uint8_t * byteMask, int32_t isSuffix) {
    int32_t charMaskLength = (int32_t) strlen(charMask);
    if (charMaskLength == 0) {
        printf("Error parsing mask. Mask is empty.\n");
        exit(1);
    }
    if (charMaskLength > 24) {
        printf("Error parsing mask. Mask is too big.\n");
        exit(1);
    }
    for (size_t i = 0; i < 17; i++) {
        byteMask[i] = MASK_MATCH_ANY;
    }
    if (isSuffix == 1) {
        int32_t byteMaskIndex = RS_ADDRESS_BYTE_SIZE - 1;
        int32_t i = charMaskLength - 1;
        while (i >= 0) {
            if (byteMaskIndex < 0) {
                printf("Error parsing mask. Mask is too big..\n");
                exit(1);
            }
            byteMask[byteMaskIndex] = rscharToIndex(charMask[i]);
            if (byteMask[byteMaskIndex] == MASK_MATCH_MINUS) {
                byteMask[byteMaskIndex] = MASK_MATCH_ANY;
                byteMaskIndex++;
            }
            i--;
            byteMaskIndex--;
        }
    } else {
        int32_t byteMaskIndex = 0;
        int32_t i = 0;
        while (i < charMaskLength) {
            if (byteMaskIndex >= RS_ADDRESS_BYTE_SIZE) {
                printf("Error parsing mask. Mask is too big...\n");
                exit(1);
            }
            byteMask[byteMaskIndex] = rscharToIndex(charMask[i]);
            if (byteMask[byteMaskIndex] == MASK_MATCH_MINUS) {
                byteMask[byteMaskIndex] = MASK_MATCH_ANY;
                byteMaskIndex--;
            }
            i++;
            byteMaskIndex++;
        }
    }
    if (byteMask[12] >= MASK_MATCH_J && byteMask[12] <= MASK_MATCH_Z) {
        printf("Error parsing mask. Char '%c' is not allowed on position 13. Use only [23456789ABCDEFGH].\n", rsAlphabet[byteMask[12]]);
        exit(1);
    }
}

uint8_t matchMask(const uint8_t * byteMask, const uint8_t * account) {
    for (size_t i = 0; i < RS_ADDRESS_BYTE_SIZE; i++) {
        switch (byteMask[i]) {
        case MASK_MATCH_ANY:
            continue;
        case MASK_MATCH_ANY_NUMBER:
            if (account[i] >= MASK_MATCH_A) return 0;
            continue;
        case MASK_MATCH_ANY_LETTER:
            if (account[i] < MASK_MATCH_A) return 0;
            continue;
        default:
            if (byteMask[i] != account[i]) return 0;
        }
    }
    return 1;
}

double findingChance(uint8_t * byteMask) {
    double events = 1.0;
    for (size_t i = 0; i < RS_ADDRESS_BYTE_SIZE; i++) {
        switch (byteMask[i]) {
        case MASK_MATCH_ANY:
            break;
        case MASK_MATCH_ANY_NUMBER:
            if (i==12) events *= 16.0/8.0;
            else events *= 32.0/8.0;
            break;
        case MASK_MATCH_ANY_LETTER:
            if (i==12) events *= 16.0/8.0;
            else events *= 32.0/24.0;
            break;
        default:
            // It is a char that was fixed
            if (i==12) events *= 16.0;
            else events *= 32.0;
            break;
        }
    }
    return (1.0/events);
}

void byteMaskToPrintMask(uint8_t * byteMask, char * printMask) {
    for (size_t i = 0, j=0; i < RS_ADDRESS_BYTE_SIZE; i++, j++) {
        if (j==4 || j==9 || j==14) {
            printMask[j] = '-';
            j++;
        }
        printMask[j] = rsMaskAlphabet[byteMask[i]];
    }
    printMask[RS_ADDRESS_STRING_SIZE - 1] = '\0';
}
