#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/bn.h>

#include "ed25519-donna/ed25519.h"
#include "globalTypes.h"
#include "cpu.h"
#include "ReedSolomon.h"

extern struct CONFIG GlobalConfig;

/* Transforms a public key in unsigned long ID */
uint64_t hashToId(const uint8_t * pk) {
    uint64_t retVal = pk[0];
    retVal |= ((uint64_t) pk[1]) << 8;
    retVal |= ((uint64_t) pk[2]) << 16;
    retVal |= ((uint64_t) pk[3]) << 24;
    retVal |= ((uint64_t) pk[4]) << 32;
    retVal |= ((uint64_t) pk[5]) << 40;
    retVal |= ((uint64_t) pk[6]) << 48;
    retVal |= ((uint64_t) pk[7]) << 56;
    return retVal;
}

uint8_t * cpuInit(void) {
    uint8_t * resultBuffer;
    resultBuffer = (uint8_t *) malloc(GlobalConfig.gpuThreads * sizeof(uint8_t));
    if (resultBuffer == NULL) {
        printf("Cannot allocate memory for result buffer\n");
        exit(1);
    }

#ifdef OS_LINUX
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    if (cpuinfo != NULL) {
        char *arg = 0;
        size_t size = 0;
        while (getline(&arg, &size, cpuinfo) != -1) {
            if (strncmp(arg, "model name", 10) == 0) {
                printf("Calculating using the cpu %s", arg);
                free(arg);
                fclose(cpuinfo);
                return resultBuffer;
            }
        }
        free(arg);
        fclose(cpuinfo);
    }
#endif
    printf("Calculating using the cpu...\n");
    return resultBuffer;
}

void cpuSolver(const struct PASSPHRASE * passphraseBatch, uint8_t * foundBatch) {
    uint8_t publicKey[SHA256_DIGEST_LENGTH];
    uint8_t privateKey[SHA256_DIGEST_LENGTH];
    uint8_t fullId[SHA256_DIGEST_LENGTH];
    uint8_t rsAccount[RS_ADDRESS_BYTE_SIZE];
    SHA256_CTX ctx;

    for (size_t i = 0; i < GlobalConfig.gpuThreads; i++) {
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, passphraseBatch[i].string + passphraseBatch[i].offset, PASSPHRASE_MAX_LENGTH - passphraseBatch[i].offset);
        SHA256_Final(privateKey, &ctx);
        curved25519_scalarmult_basepoint(publicKey, privateKey);
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
        SHA256_Final(fullId, &ctx);
        uint64_t id = hashToId(fullId);
        idToByteAccount(id, rsAccount);
        foundBatch[i] = matchMask(GlobalConfig.mask, rsAccount);
    }
}

/** Expecting a public key in uint8_t array format (32 bytes) and
 *  a preallocated buffer publicKeyHex for hex string output
 *  (65 chars) */
void publicKeyToHexString(uint8_t * publicKey, char * publicKeyHex) {
    size_t i = 0;
    char base16[] = "0123456789abcdef";
    while (i < 64) {
        uint8_t val = publicKey[i / 2];
        publicKeyHex[i] = base16[val / 16];
        i++;
        publicKeyHex[i] = base16[val % 16];
        i++;
    }
    publicKeyHex[64] = '\0';
}

/** Expecting a public key in hex string format (64 chars) and
 *  a preallocated buffer extendedPublicKey for base36 output
 *  (51 chars) */
void hexPKToExtendedPK(const char * publicKeyHex, char * extendedPublicKey) {
    size_t i;
    BN_CTX *ctx;
    BIGNUM *dividend = NULL;
    BIGNUM *newRemainder = NULL;
    BIGNUM *remainder = NULL;
    BIGNUM *divisor = NULL;
    BIGNUM *base = NULL;
    char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    ctx = BN_CTX_new();
    dividend = BN_new();
    newRemainder = BN_new();

    // remainder = hexToBn(publicKeyHex)
    BN_hex2bn(&remainder, publicKeyHex);
    // divisor = 18147739541668636280463618532168272792698436402026524209529776843597142818816 :: (36^49)
    BN_dec2bn(&divisor, "18147739541668636280463618532168272792698436402026524209529776843597142818816");
    // base = 36
    BN_dec2bn(&base, "36");
    i = 0;
    while (!BN_is_zero(divisor)) {
        // dividend = remainder / divisor
        // newRemainder = remainder % divisor
        BN_div(dividend, remainder, remainder, divisor, ctx);
        unsigned long dividend_ulong = BN_get_word(dividend);
        if (dividend_ulong != 0 || i != 0) {
            extendedPublicKey[i] = alphabet[dividend_ulong];
            i++;
        }
        // divisor /= 36
        BN_div(divisor, NULL, divisor, base, ctx);
    }
    if (i==0) {
        extendedPublicKey[i] = alphabet[0];
        i++;
    }
    extendedPublicKey[i] = '\0';
    
    BN_free(dividend);
    BN_free(newRemainder);
    BN_free(remainder);
    BN_free(divisor);
    BN_free(base);
    BN_CTX_free(ctx);
}

uint64_t solveOnlyOne(const struct PASSPHRASE * passphrase, char * rsAddress, char * publicKeyHex, char * extendedPublicKey) {
    uint8_t publicKey[SHA256_DIGEST_LENGTH];
    uint8_t privateKey[SHA256_DIGEST_LENGTH];
    uint8_t fullId[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    uint64_t id;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, passphrase->string + passphrase->offset, PASSPHRASE_MAX_LENGTH - passphrase->offset);
    SHA256_Final(privateKey, &ctx);
    curved25519_scalarmult_basepoint(publicKey, privateKey);
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
    SHA256_Final(fullId, &ctx);
    id = hashToId(fullId);
    idToAccount(id, rsAddress);
    if (publicKeyHex != NULL && extendedPublicKey != NULL) {
        publicKeyToHexString(publicKey, publicKeyHex);
        hexPKToExtendedPK(publicKeyHex, extendedPublicKey);
    }
    return id;
}
