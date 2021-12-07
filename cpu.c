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

void cpuInit(void) {
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
                return;
            }
        }
        free(arg);
        fclose(cpuinfo);
    }
#endif
    printf("Calculating using the cpu...\n");
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
