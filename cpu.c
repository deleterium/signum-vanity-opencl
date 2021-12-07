#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "ed25519-donna/ed25519.h"
#include "cpu.h"
#include "globalTypes.h"
#include "ReedSolomon.h"

extern struct CONFIG GlobalConfig;

/* Transforms a public key in unsigned long ID */
unsigned long hashToId(const BYTE * pk) {
    unsigned long retVal = pk[0];
    retVal |= ((long) pk[1]) << 8;
    retVal |= ((long) pk[2]) << 16;
    retVal |= ((long) pk[3]) << 24;
    retVal |= ((long) pk[4]) << 32;
    retVal |= ((long) pk[5]) << 40;
    retVal |= ((long) pk[6]) << 48;
    retVal |= ((long) pk[7]) << 56;
    return retVal;
}

unsigned char * cpuInit(void) {
    unsigned char * resultBuffer;
    resultBuffer = (unsigned char *) malloc(GlobalConfig.gpuThreads * sizeof(unsigned char));
    if (resultBuffer == NULL) {
        printf("Cannot allocate memory for result buffer\n");
        exit(1);
    }

#ifdef LINUX
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    if (cpuinfo != NULL) {
        char *arg = 0;
        size_t size = 0;
        while(getline(&arg, &size, cpuinfo) != -1) {
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


/**
 * Transforms a batch of passphrases to ID's using the cpu */
void cpuSolver(const char* passphraseBatch, unsigned char * foundBatch) {
    BYTE publicKey[SHA256_DIGEST_LENGTH];
    BYTE privateKey[SHA256_DIGEST_LENGTH];
    BYTE fullId[SHA256_DIGEST_LENGTH];
    BYTE rsAccount[RS_ADDRESS_BYTE_SIZE];
    SHA256_CTX ctx;

    for (int i=0; i< GlobalConfig.gpuThreads; i++) {
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, passphraseBatch + GlobalConfig.secretLength*i, GlobalConfig.secretLength);
        SHA256_Final(privateKey, &ctx);
        curved25519_scalarmult_basepoint(publicKey, privateKey);
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
        SHA256_Final(fullId, &ctx);
        unsigned long id = hashToId(fullId);
        idTOByteAccount(id,rsAccount);
        foundBatch[i] = matchMask(GlobalConfig.mask, rsAccount);
    }
}

unsigned long solveOnlyOne(const char* passphrase, char* rsAddress){
    BYTE publicKey[SHA256_DIGEST_LENGTH];
    BYTE privateKey[SHA256_DIGEST_LENGTH];
    BYTE fullId[SHA256_DIGEST_LENGTH];
    BYTE rsAccount[RS_ADDRESS_BYTE_SIZE];
    SHA256_CTX ctx;
    unsigned long id;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, passphrase, GlobalConfig.secretLength);
    SHA256_Final(privateKey, &ctx);
    curved25519_scalarmult_basepoint(publicKey, privateKey);
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
    SHA256_Final(fullId, &ctx);
    id = hashToId(fullId);
    idTOAccount(id, rsAddress);
    return id;
}
