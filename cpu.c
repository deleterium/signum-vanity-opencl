#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

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
        SHA256_Update(&ctx, passphraseBatch[i].string, passphraseBatch[i].length);
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

uint64_t solveOnlyOne(const struct PASSPHRASE * passphrase, char * rsAddress){
    uint8_t publicKey[SHA256_DIGEST_LENGTH];
    uint8_t privateKey[SHA256_DIGEST_LENGTH];
    uint8_t fullId[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    uint64_t id;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, passphrase->string, passphrase->length);
    SHA256_Final(privateKey, &ctx);
    curved25519_scalarmult_basepoint(publicKey, privateKey);
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
    SHA256_Final(fullId, &ctx);
    id = hashToId(fullId);
    idToAccount(id, rsAddress);
    return id;
}
