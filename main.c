/** 1 to inspect the IDs of one batch processing.
 * 0 to regular operation */
#define MDEBUG 0

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "globalTypes.h"
#include "argumentsParser.h"
#include "cpu.h"
#include "gpu.h"
#include "ReedSolomon.h"

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

float estimateTries(BYTE * byteMask) {
    float ret = 1.0;
    for (size_t i = 0; i< RS_ADDRESS_BYTE_SIZE; i++) {
        if (byteMask[i] != 32) ret *= 32;
    }
    return (-1.0 / log10((1.0 - (1.0/ret))));
}

int main(int argc, char **argv) {
    unsigned char * ID;
    char * secret;
    struct timeval tstart, tend;
    long seconds, micros;
    double timeInterval;
    unsigned long roundsToPrint = 1;

    srand(time(NULL) * 80 + 34634);
    unsigned long end = 0;
    int i;

    argumentsParser(argc, argv);

    maskToByteMask(argv[argc - 1], GlobalConfig.mask);

    secret = (char *) malloc(GlobalConfig.secretLength * GlobalConfig.gpuThreads);
    if (secret == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
    }

    for (i = 0; i < GlobalConfig.gpuThreads; i++) {
        randstring(&secret[GlobalConfig.secretLength*i], GlobalConfig.secretLength);
    }

    if (GlobalConfig.useGpu) {
        ID = gpuInit();
    } else {
        ID = cpuInit();
    }

    float halfLife = estimateTries(GlobalConfig.mask);
    printf(" %.0f tries for 90%% chance finding a match. Ctrl + C to cancel.\n", halfLife);

    unsigned long rounds = 0;
    unsigned long previousRounds = 0;
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
            unsigned long currentTries = rounds*GlobalConfig.gpuThreads;
            if (GlobalConfig.endless == 0) {
                printf(
                    "\r %lu tries - %.1f%% - %.0f tries/second...",
                    currentTries,
                    100.0 * currentTries / halfLife,
                    (float)((rounds - previousRounds) * GlobalConfig.gpuThreads) / timeInterval
                );
                fflush(stdout);
            }
            gettimeofday(&tstart, NULL);
            // adjust rounds To print
            if (timeInterval < 0.3) {
                roundsToPrint *= 2;
            }
            if (timeInterval > 1) {
                roundsToPrint /= 2;
                if (roundsToPrint == 0) roundsToPrint++;
            }
            previousRounds = rounds;
        }
        for (i=0; i< GlobalConfig.gpuThreads; i++) {
            if (ID[i] == 1) {
                if (GlobalConfig.endless == 0) {
                    end = 1;
                    printf("\nFound in %lu tries\n", rounds * GlobalConfig.gpuThreads);
                }
                // printf("\rPassphrase: '%*.*s' id: %20lu RS: %s\n", GlobalConfig.secretLength, GlobalConfig.secretLength, secret + i*GlobalConfig.secretLength, ID[i], RSAddress);
                printf("\rPassphrase: '%*.*s'\n", GlobalConfig.secretLength, GlobalConfig.secretLength, secret + i*GlobalConfig.secretLength);
                fflush(stdout);
            }
        }
#if MDEBUG == 0
    } while (end == 0);
#else
    for (i=0; i< GlobalConfig.gpuThreads; i++) {
        printf("'%*.*s': %x\n", GlobalConfig.secretLength, GlobalConfig.secretLength, secret + i*GlobalConfig.secretLength, ID[i]);
    }
#endif
}
