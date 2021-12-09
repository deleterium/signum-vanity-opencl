#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ReedSolomon.h"

#define ginv(a) (gexp[31 - glog[a]])

const BYTE gexp[] = {
    1, 2, 4, 8, 16, 5, 10, 20, 13, 26, 17, 7, 14, 28, 29, 31,
    27, 19, 3, 6, 12, 24, 21, 15, 30, 25, 23, 11, 22, 9, 18, 1
};
const BYTE glog[] = {
    0, 0, 1, 18, 2, 5, 19, 11, 3, 29, 6, 27, 20, 8, 12, 23,
    4, 10, 30, 17, 7, 22, 28, 26, 21, 25, 9, 16, 13, 14, 24, 15
};
const BYTE cwmap[] = {3, 2, 1, 0, 7, 6, 5, 4, 13, 14, 15, 16, 12, 8, 9, 10, 11};

BYTE gmult(BYTE a, BYTE b) {
    if (a == 0 || b == 0) {
        return 0;
    }
    return gexp[(glog[a] + glog[b]) % 31];
}

void idTOByteAccount(unsigned long accountId, BYTE * out) {
    BYTE codeword[RS_ADDRESS_BYTE_SIZE];
    const BYTE base32Length = 13;
    BYTE p[] = {0, 0, 0, 0};
    size_t i;
    for (i = 0; i < base32Length; i++){
        codeword[i] = accountId % 32;
        accountId >>= 5;
    }
    for (i = base32Length - 1; i < base32Length; i--) {
        BYTE fb = codeword[i] ^ p[3];
        p[3] = p[2] ^ gmult(30, fb);
        p[2] = p[1] ^ gmult(6, fb);
        p[1] = p[0] ^ gmult(9, fb);
        p[0] = gmult(17, fb);
    }
    codeword[13] = p[0];
    codeword[14] = p[1];
    codeword[15] = p[2];
    codeword[16] = p[3];
    for (i = 0; i < RS_ADDRESS_BYTE_SIZE; i++) {
        out[i] = codeword[cwmap[i]];
    }
}

void idTOAccount(unsigned long accountId, char * out) {
    BYTE byteOut[RS_ADDRESS_BYTE_SIZE];
    const char alphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    idTOByteAccount(accountId, byteOut);
    for (size_t i = 0; i < RS_ADDRESS_BYTE_SIZE; i++) {
        out[i] = alphabet[byteOut[i]];
    }
    out[RS_ADDRESS_BYTE_SIZE] = '\0';
}

BYTE rscharToIndex(char in) {
    switch (in) {
    case '2': return 0;
    case '3': return 1;
    case '4': return 2;
    case '5': return 3;
    case '6': return 4;
    case '7': return 5;
    case '8': return 6;
    case '9': return 7;
    case 'A': return 8;
    case 'B': return 9;
    case 'C': return 10;
    case 'D': return 11;
    case 'E': return 12;
    case 'F': return 13;
    case 'G': return 14;
    case 'H': return 15;
    case 'J': return 16;
    case 'K': return 17;
    case 'L': return 18;
    case 'M': return 19;
    case 'N': return 20;
    case 'P': return 21;
    case 'Q': return 22;
    case 'R': return 23;
    case 'S': return 24;
    case 'T': return 25;
    case 'U': return 26;
    case 'V': return 27;
    case 'W': return 28;
    case 'X': return 39;
    case 'Y': return 30;
    case 'Z': return 31;
    case '_': return 32;
    default:
        printf("Error parsing mask. Char '%c' is not allowed.  Try '--help'.\n", in);
        exit(1) ;
    }
}

void maskToByteMask(const char * charMask, BYTE * byteMask, int isSuffix) {
    char processedMask[18];

    if (strlen(charMask) == 0) {
        printf("Error parsing mask. Mask is empty.\n");
        exit(1);
    }
    if (strlen(charMask) > 17) {
        printf("Error parsing mask. Mask is too big.\n");
        exit(1);
    }
    if (isSuffix == 1) {
        strcpy(processedMask, "_________________");
        strcpy(processedMask+17-strlen(charMask), charMask);
    } else {
        strcpy(processedMask, charMask);
    }
    for (size_t i = 0; i < 17; i++) {
        byteMask[i] = 32;
    }
    for (size_t i = 0; i < strlen(processedMask); i++) {
        byteMask[i] = rscharToIndex(processedMask[i]);
    }
    if (byteMask[12] > 15 && byteMask[12] != 32) {
        printf("Error parsing mask. Char '%c' is not allowed on position 13.\n", byteMask[12]);
        exit(1);
    }
}

unsigned char matchMask(const BYTE * byteMask, const BYTE * account) {
    for (size_t i = 0; i < 17; i++) {
        if (byteMask[i] == 32) continue;
        if (byteMask[i] == account[i]) continue;
        return 0;
    }
    return 1;
}
