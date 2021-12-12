/** 1 to inspect the IDs of one batch processing.
 * 0 to regular operation */
#define MDEBUG 0

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "globalTypes.h"
#include "argumentsParser.h"
#include "cpu.h"
#include "gpu.h"
#include "ReedSolomon.h"

struct CONFIG GlobalConfig;

#ifdef OS_WINDOWS
#include <Windows.h>
#include <sysinfoapi.h>
#define CLOCK_REALTIME 0
int clock_gettime(int noNeed, struct timespec* spec) {
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime -= 116444736000000000i64;  //1jan1601 to 1jan1970
    spec->tv_sec = wintime / 10000000i64; //seconds
    spec->tv_nsec = wintime % 10000000i64 * 100; //nano-seconds
    return 0;
}
#endif

/**
 * Creates a random string inside buffer, with length size. The generated
 * string will contain ASCII characters from '!' to 'z'. Mininium 40 chars
 * to have 256-bit output. */
void randstring(char * buffer, size_t length) {
    if (GlobalConfig.charset[0] == 0) {
        for (size_t n = 0; n < length; n++) {
            char key = rand() % 89;
            buffer[n] = '!' + key;
        }
    } else {
        int32_t charsetLength = (int32_t)strlen(GlobalConfig.charset);
        for (size_t n = 0; n < length; n++) {
            size_t key = rand() % charsetLength;
            buffer[n] = GlobalConfig.charset[key];
        }
    }
}

/**
 * Increments one char at secret buffer. No buffer overflow is checked. */
void incSecret(char * secret, size_t position) {
    if (GlobalConfig.charset[0] == 0) {
        if (secret[position] >= 'z') {
            secret[position] = '!';
            incSecret(secret, position + 1);
            return;
        }
        secret[position] += 1;
    } else {
        char * currCharHeight = strchr(GlobalConfig.charset, (int)secret[position]);
        if (currCharHeight == NULL) {
            printf("Internal error\n");
            exit(2);
        }
        size_t height = (size_t)(currCharHeight - GlobalConfig.charset);
        if (height == strlen(GlobalConfig.charset) - 1) {
            secret[position] = GlobalConfig.charset[0];
            incSecret(secret, position + 1);
            return;
        }
        secret[position] = GlobalConfig.charset[height + 1];
    }
}

void initRand(void) {
    uint32_t randomData;
#ifdef OS_LINUX
    FILE *random = fopen("/dev/random", "rb");
    if (random != NULL) {
        if (fread (&randomData,sizeof(randomData), 1, random) == 1) {
            srand(randomData);
            printf("Got random seed from /dev/random!\n");
            fclose(random);
            return;

        }
        fclose(random);
    }
#endif
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) == 0) {
        randomData = (uint32_t)(now.tv_sec ^ now.tv_nsec);
        srand(randomData);
        printf("Got random seed from current microseconds.\n");
        return;
    }
    srand((uint32_t)time(NULL) * 80 + 34634);
    printf("Not good.. Got random seed from current second...\n");
}

double getPassphraseStrength(void) {
    size_t i;
    char * foundChar;
    double passStrength;
    if (GlobalConfig.charset[0] == 0) {
        passStrength = log2(pow(89.0, (double)GlobalConfig.secretLength));
    } else {
        size_t charsetLength = strlen(GlobalConfig.charset);
        for (i=0; i< charsetLength; i++) {
            if (i < charsetLength - 1) {
                foundChar = strchr(GlobalConfig.charset+i+1, (int)GlobalConfig.charset[i]);
                if (foundChar != NULL) {
                    printf("Wrong charset. Found a repeated char.\n");
                    exit(1);
                }
            }
            if (GlobalConfig.charset[i] < 32 || GlobalConfig.charset[i] > 126) {
                printf("No special (unicode) chars are allowed in charset.\n");
                exit(1);
            }
        }
        passStrength = log2(pow((double)strlen(GlobalConfig.charset), (double)GlobalConfig.secretLength));
    }
    if (passStrength < 256.0) {
        printf("Weak passphrase detected. It is %.f bits strong. It must be greater than 256 bits. Increase pass-length or increase charset length.\n", passStrength);
        exit(1);
    }
    return passStrength;
}

double estimate90percent(double findingChance) {
    return (-1.0 / log10((1.0 - findingChance)));
}

double luckyChance(double numberOfEvents, double findingChance) {
    return (1.0 - pow(1.0 - findingChance,numberOfEvents)) * 100.0;
}

int main(int argc, char ** argv) {
    uint8_t * ID;
    char * secret;
    char printMask[RS_ADDRESS_STRING_SIZE];
    struct timespec tstart, tend;
    int64_t seconds, nanos;
    double eventChance;
    double timeInterval;
    uint64_t roundsToPrint = 1;
    uint64_t end = 0;
    uint64_t rounds = 0;
    uint64_t previousRounds = 0;
    int maskIndex;

    size_t i;

    initRand();
    maskIndex = argumentsParser(argc, argv);
    maskToByteMask(argv[maskIndex], GlobalConfig.mask, GlobalConfig.suffix);
    secret = (char *) malloc(GlobalConfig.secretLength * GlobalConfig.gpuThreads);
    if (secret == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
    }
    randstring(secret, GlobalConfig.secretLength * GlobalConfig.gpuThreads);
    if (GlobalConfig.useGpu) {
        ID = gpuInit();
    } else {
        ID = cpuInit();
    }

    byteMaskToPrintMask(GlobalConfig.mask, printMask);
    printf("Using MASK %s\n", printMask);
    printf("Your passphrase will be %.f bits strong!\n", getPassphraseStrength());
    eventChance = findingChance(GlobalConfig.mask);
    printf(" %.0f tries for 90%% chance finding a match. Ctrl + C to cancel.\n", estimate90percent(eventChance));

    clock_gettime(CLOCK_REALTIME, &tstart);
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
        if ((rounds % roundsToPrint) == 0) {
            clock_gettime(CLOCK_REALTIME, &tend);
            seconds = (tend.tv_sec - tstart.tv_sec);
            nanos = ((seconds * 1000000000LL) + tend.tv_nsec) - tstart.tv_nsec;
            timeInterval = (double) nanos / 1000000000.0;
            uint64_t currentTries = rounds * GlobalConfig.gpuThreads;
            printf(
                "\r %llu tries - Lucky chance: %.1f%% - %.0f tries/second...",
                PRINTF_CAST currentTries,
                luckyChance((double)currentTries, eventChance),
                (double) ((rounds - previousRounds) * GlobalConfig.gpuThreads) / timeInterval
            );
            fflush(stdout);
            clock_gettime(CLOCK_REALTIME, &tstart);
            // adjust rounds To print
            if (timeInterval < 0.3) {
                roundsToPrint *= 2;
            }
            if (timeInterval > 1.0) {
                roundsToPrint /= 2;
                if (roundsToPrint == 0) roundsToPrint++;
            }
            previousRounds = rounds;
        }
        for (i = 0; i < GlobalConfig.gpuThreads; i++) {
            if (ID[i] == 1) {
                char rsAddress[RS_ADDRESS_STRING_SIZE];
                uint64_t newId = solveOnlyOne(secret + i * GlobalConfig.secretLength, rsAddress);
                if (GlobalConfig.endless == 0) {
                    printf(
                        "\nPassphrase: '%*.*s' RS: S-%s id: %20llu\n",
                        (int)GlobalConfig.secretLength,
                        (int)GlobalConfig.secretLength,
                        secret + i * GlobalConfig.secretLength,
                        rsAddress,
                        PRINTF_CAST newId
                    );
                    fflush(stdout);
                    end = 1;
                    break;
                } else {
                    printf(
                        "\rPassphrase: '%*.*s' RS: S-%s id: %20llu\n",
                        (int)GlobalConfig.secretLength,
                        (int)GlobalConfig.secretLength,
                        secret + i * GlobalConfig.secretLength,
                        rsAddress,
                        PRINTF_CAST newId
                    );
                    fflush(stdout);
                    rounds = 0;
                    previousRounds = 0;
                }
            }
        }
#if MDEBUG == 0
    } while (end == 0);
#else
    for (i = 0; i < GlobalConfig.gpuThreads; i++) {
        printf(
            "'%*.*s': %x\n",
            (int)GlobalConfig.secretLength,
            (int)GlobalConfig.secretLength,
            secret + i * GlobalConfig.secretLength,
            ID[i]
        );
    }
#endif
}
