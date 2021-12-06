/** 1 to inspect the IDs of one batch processing.
 * 0 to regular operation */
#define MDEBUG 0

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include <sys/time.h>

#include "globalTypes.h"
#include "argumentsParser.h"
#include "programConstants.h"
#include "gpu.h"
#include "ed25519-donna/ed25519.h"

struct CONFIG GlobalConfig;

/**
 * Creates a random string inside buffer, with length size. The generated
 * string will contain ASCII characters from '!' to 'z'. Mininium 40 chars
 * to have 256-bit output. */
void randstring(char * buffer, size_t length) {
    if (length) {
        int n;
        for (n = 0;n < length;n++) {
            char key = rand() % 89;
            buffer[n] = '!' + key;
        }
    }
}

/**
 * Increments one char at secret buffer. No buffer overflow is checked. */
void incSecret(char * secret, size_t position) {
    if (secret[position] >= 'z') {
        secret[position] = '!';
        incSecret(secret, position + 1);
        return;
    }
    secret[position] += 1;
}

/**
 * Transforms a public key in insigned long ID */
unsigned long hashToId(const unsigned char * pk) {
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

/**
 * Transforms a batch of passphrases to ID's using the cpu */
void cpuSolver(const char* passphraseBatch, unsigned long * resultBatch) {
    unsigned char publicKey[SHA256_DIGEST_LENGTH];
    unsigned char privateKey[SHA256_DIGEST_LENGTH];
    unsigned char fullId[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;

    for (int i=0; i< GlobalConfig.gpuThreads; i++) {
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, passphraseBatch + GlobalConfig.secretLength*i, GlobalConfig.secretLength);
        SHA256_Final(privateKey, &ctx);
        curved25519_scalarmult_basepoint(publicKey, privateKey);
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
        SHA256_Final(fullId, &ctx);
        resultBatch[i] = hashToId(fullId);
    }
}

int main(int argc, char **argv) {
    unsigned long * ID;
    char * secret; //[GlobalConfig.secretLength * batchSize];
    struct timeval tstart, tend;
    long seconds, micros;
    double timeInterval;

    srand(time(NULL) * 80 + 34634);
    unsigned long end = 0;
    int i;

    argumentsParser(argc, argv);

    ID = (unsigned long *) malloc(GlobalConfig.gpuThreads * sizeof(unsigned long));
    if (ID == NULL) {
        printf("Cannot allocate memory for result buffer\n");
        exit(1);
    }

    secret = (char *) malloc(GlobalConfig.secretLength * GlobalConfig.gpuThreads);
    if (ID == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
    }

#if MDEBUG == 1
    memcpy(secret,                  "pWME8qhktupYnNxwhxLVr7rUB1UpXqkcaUG7aMhu6vgh6CEdZdYfadd", GlobalConfig.secretLength);
    memcpy(secret+GlobalConfig.secretLength, "ly5py9cW4LBKxmvjTK0nzOdkY6a3qvaAT4Nf1P1VpqtMCOTkmJxLnzd", GlobalConfig.secretLength);
    memcpy(secret+2*GlobalConfig.secretLength,"Fy5dJuXJNHCnKtlwdWnPd49H2OLq5AYxWTBuco3NTvYrM9LPWYtXRd", GlobalConfig.secretLength);
    i=3;
#else
    i=0;
#endif
    for (; i < GlobalConfig.gpuThreads; i++) {
        randstring(&secret[GlobalConfig.secretLength*i], GlobalConfig.secretLength);
    }

    // Remember to hardcode batchSize at programConstants.h
    if (GlobalConfig.useGpu) {
        gpuInit();
    }

    printf("Searching...\n");

    unsigned long rounds = 0;
    gettimeofday(&tstart, NULL);
#if MDEBUG == 0
    do {
        for (i=0; i < GlobalConfig.gpuThreads; i++) {
            incSecret(secret+GlobalConfig.secretLength*i, 0);
        }
#endif
        if (GlobalConfig.useGpu) {
            gpuSolver(secret, ID);
        } else {
            cpuSolver(secret, ID);
        }
        ++rounds;
        if ( (rounds % roundsToPrint) == 0L) {
            gettimeofday(&tend, NULL);
            seconds = (tend.tv_sec - tstart.tv_sec);
            micros = ((seconds * 1000000) + tend.tv_usec) - (tstart.tv_usec);
            timeInterval = (double)micros/1000000;
            printf("\r %lu: %f tries/second in %f seconds", rounds*GlobalConfig.gpuThreads, (double)(roundsToPrint*GlobalConfig.gpuThreads)/ timeInterval, timeInterval);
            fflush(stdout);
            gettimeofday(&tstart, NULL);
        }
        for (i=0; i< GlobalConfig.gpuThreads; i++) {
            if (ID[i] > 0xffffff0000000000) {
                end = 1;
                break;
            }
        }
#if MDEBUG == 0
    } while (end == 0);
    printf("\nFound in %lu rounds\n", rounds * GlobalConfig.gpuThreads);
    printf("\nPassphrase: '%*.*s' id: %lu\n", GlobalConfig.secretLength, GlobalConfig.secretLength, secret + i*GlobalConfig.secretLength, ID[i]);
#else
    for (i=0; i< batchSize; i++) {
        printf("'%*.*s': %lu\n", GlobalConfig.secretLength, GlobalConfig.secretLength, secret + i*GlobalConfig.secretLength, ID[i]);
    }
#endif
}
