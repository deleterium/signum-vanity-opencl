// #define MDEBUG

#include "programConstants.h"
#include "gpu.h"
#include "ed25519-donna/ed25519.h"
#include <openssl/sha.h>
#include <time.h>
#include <sys/time.h>

void randstring(char * buffer, size_t length) {
    if (length) {
        int n;
        for (n = 0;n < length;n++) {
            char key = rand() % 89;
            buffer[n] = '!' + key;
        }
    }
}

void incSecret(char * secret, size_t position) {
    if (secret[position] >= 'z') {
        secret[position] = '!';
        incSecret(secret, position + 1);
        return;
    }
    secret[position] += 1;
}

unsigned long hashToId(unsigned char * bytes) {
    unsigned long retVal = bytes[0];
    retVal |= ((long) bytes[1]) << 8;
    retVal |= ((long) bytes[2]) << 16;
    retVal |= ((long) bytes[3]) << 24;
    retVal |= ((long) bytes[4]) << 32;
    retVal |= ((long) bytes[5]) << 40;
    retVal |= ((long) bytes[6]) << 48;
    retVal |= ((long) bytes[7]) << 56;
    return retVal;
}

void cpuSolver(char* input, unsigned long * result) {
    unsigned char publicKey[SHA256_DIGEST_LENGTH];
    unsigned char privateKey[SHA256_DIGEST_LENGTH];
    unsigned char fullId[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;

    for (int i=0; i< batchSize; i++) {
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, input + secretBufferSize*i, secretBufferSize);
        SHA256_Final(privateKey, &ctx);
        curved25519_scalarmult_basepoint(publicKey, privateKey);
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, publicKey, SHA256_DIGEST_LENGTH);
        SHA256_Final(fullId, &ctx);
        result[i] = hashToId(fullId);
    }
}

int main() {
    unsigned long ID[batchSize];
    char secret[secretBufferSize * batchSize];
    struct timeval tstart, tend;
    long seconds, micros;
    double timeInterval;

    srand(time(NULL) * 80 + 34634);
    bool end = false;
    bool useGpu = true;

    int i;

#ifdef MDEBUG
    memcpy(secret,                  "pWME8qhktupYnNxwhxLVr7rUB1UpXqkcaUG7aMhu6vgh6CEdZdYfadd", secretBufferSize);
    memcpy(secret+secretBufferSize, "ly5py9cW4LBKxmvjTK0nzOdkY6a3qvaAT4Nf1P1VpqtMCOTkmJxLnzd", secretBufferSize);
    memcpy(secret+2*secretBufferSize,"Fy5dJuXJNHCnKtlwdWnPd49H2OLq5AYxWTBuco3NTvYrM9LPWYtXRd", secretBufferSize);
    i=3;
#else
    i=0;
#endif
    for (; i < batchSize; i++) {
        randstring(&secret[secretBufferSize*i], secretBufferSize);
    }

    // Remember to hardcode batchSize at programConstants.h
    if (useGpu) {
        gpuInit();
    }

    printf("Searching...\n");

    unsigned long rounds = 0;
    gettimeofday(&tstart, NULL);
#ifndef MDEBUG
    do {
        for (i=0; i < batchSize; i++) {
            incSecret(secret+secretBufferSize*i, 0);
        }
#endif
        if (useGpu) {
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
            printf("\r %lu: %f tries/second in %f seconds", rounds*batchSize, (double)(roundsToPrint*batchSize)/ timeInterval, timeInterval);
            fflush(stdout);
            gettimeofday(&tstart, NULL);
        }
        for (i=0; i< batchSize; i++) {
            if (ID[i] > 0xffffff0000000000) {
                end = true;
                break;
            }
        }
#ifndef MDEBUG
    } while (end == false);
    printf("\nFound in %lu rounds\n", rounds * batchSize);
    printf("\nPassphrase: '%*.*s' id: %lu\n", secretBufferSize, secretBufferSize, secret + i*secretBufferSize, ID[i]);
#else
    for (i=0; i< batchSize; i++) {
        printf("'%*.*s': %lu\n", secretBufferSize, secretBufferSize, secret + i*secretBufferSize, ID[i]);
    }
#endif
}
