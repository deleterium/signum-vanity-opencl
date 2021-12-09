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
        for (n = 0; n < length; n++) {
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

float estimate90percent(float findingChance) {
    return (-1.0 / log10((1.0 - findingChance)));
}

float findingChance(BYTE * byteMask) {
    float events = 1.0;
    for (size_t i = 0; i < RS_ADDRESS_BYTE_SIZE; i++) {
        if (byteMask[i] != 32) events *= 32;
    }
    return (1.0/events);
}

float luckyChance(float numberOfEvents, float findingChance) {
    return (1.0 - pow(1.0 - findingChance,numberOfEvents)) * 100.0;
}

int main(int argc, char ** argv) {
    unsigned char * ID;
    char * secret;
    struct timeval tstart, tend;
    long seconds, micros;
    float eventChance;
    double timeInterval;
    unsigned long roundsToPrint = 1;
    unsigned long end = 0;
    unsigned long rounds = 0;
    unsigned long previousRounds = 0;

    int i;

    srand(time(NULL) * 80 + 34634);
    argumentsParser(argc, argv);
    maskToByteMask(argv[argc - 1], GlobalConfig.mask);
    secret = (char *) malloc(GlobalConfig.secretLength * GlobalConfig.gpuThreads);
    if (secret == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
    }
    for (i = 0; i < GlobalConfig.gpuThreads; i++) {
        randstring(&secret[GlobalConfig.secretLength * i], GlobalConfig.secretLength);
    }
    if (GlobalConfig.useGpu) {
        ID = gpuInit();
    } else {
        ID = cpuInit();
    }
    eventChance = findingChance(GlobalConfig.mask);
    printf(" %.0f tries for 90%% chance finding a match. Ctrl + C to cancel.\n", estimate90percent(eventChance));

    gettimeofday(&tstart, NULL);
#if MDEBUG == 0
    do {
        for (i = 0; i < GlobalConfig.gpuThreads; i++) {
            incSecret(secret + GlobalConfig.secretLength * i, 0);
        }
#endif
        if (GlobalConfig.useGpu) {
            gpuSolver(secret, ID);
        } else {
            cpuSolver(secret, ID);
        }
        ++rounds;
        if ((rounds % roundsToPrint) == 0L) {
            gettimeofday(&tend, NULL);
            seconds = (tend.tv_sec - tstart.tv_sec);
            micros = ((seconds * 1000000) + tend.tv_usec) - tstart.tv_usec;
            timeInterval = (double) micros / 1000000;
            unsigned long currentTries = rounds * GlobalConfig.gpuThreads;
            if (GlobalConfig.endless == 0) {
                printf(
                    "\r %lu tries - Lucky chance: %.1f%% - %.0f tries/second...",
                    currentTries,
                    luckyChance((float)currentTries, eventChance),
                    (float) ((rounds - previousRounds) * GlobalConfig.gpuThreads) / timeInterval
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
        for (i = 0; i < GlobalConfig.gpuThreads; i++) {
            if (ID[i] == 1) {
                char rsAddress[RS_ADDRESS_STRING_SIZE];
                unsigned long newId = solveOnlyOne(secret + i * GlobalConfig.secretLength, rsAddress);
                printf(
                    "\nPassphrase: '%*.*s' id: %20lu RS: %s",
                    GlobalConfig.secretLength,
                    GlobalConfig.secretLength,
                    secret + i * GlobalConfig.secretLength,
                    newId,
                    rsAddress
                );
                fflush(stdout);
                if (GlobalConfig.endless == 0) {
                    end = 1;
                    printf("\nFound in %lu tries\n", rounds * GlobalConfig.gpuThreads);
                    break;
                }
            }
        }
#if MDEBUG == 0
    } while (end == 0);
#else
    for (i = 0; i < GlobalConfig.gpuThreads; i++) {
        printf(
            "'%*.*s': %x\n",
            GlobalConfig.secretLength,
            GlobalConfig.secretLength,
            secret + i * GlobalConfig.secretLength,
            ID[i]
        );
    }
#endif
}
