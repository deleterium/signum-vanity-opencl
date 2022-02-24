/** 1 to inspect the IDs of one batch processing.
 * 0 to regular operation */
#define MDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "globalTypes.h"
#include "argumentsParser.h"
#include "cpu.h"
#include "gpu.h"
#include "ReedSolomon.h"
#include "dbHandler.h"

// Compiling command:
// gcc -o vanity main.c cpu.c gpu.c ReedSolomon.c ed25519-donna/ed25519.c argumentsParser.c dbHandler.c -m64 -lcrypto  -lOpenCL -lm -O1 -Wextra $(pkg-config --libs --cflags libmongoc-1.0)

struct CONFIG GlobalConfig;

void fillSecretBuffer(short * auxBuf, struct PASSPHRASE * passBuf);

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
 * Populate buffer with random number
 */
void seedRand(short * buffer, uint64_t length) {
    for (size_t n = 0; n < length; n++) {
        buffer[n] = (short)(rand() % GlobalConfig.charsetLength);
    }
}

/**
 * Increments one value at aux buffer and refresh passphrase (only changes) */
void incSecretAuxBufRegular(short * auxBuf, size_t position, struct PASSPHRASE * passBuf) {
    if (auxBuf[position] == (short) GlobalConfig.charsetLength - 1) {
        if (position == GlobalConfig.secretLength - 1) {
            for (size_t i = 0; i < GlobalConfig.secretLength; i++) {
                auxBuf[i] = 0;
                passBuf->string[passBuf->offset + i] = GlobalConfig.charset[0];
            }
            return;
        }
        auxBuf[position] = 0;
        passBuf->string[passBuf->offset + position] = GlobalConfig.charset[0];
        incSecretAuxBufRegular(auxBuf, position + 1, passBuf);
        return;
    }
    auxBuf[position]++;
    passBuf->string[passBuf->offset + position] = GlobalConfig.charset[auxBuf[position]];
    return;
}

/**
 * Increments one value at aux buffer and refresh passphrase (only changes) */
int incSecretAuxBufBip39(short * auxBuf, size_t position, struct PASSPHRASE * passBuf) {
    if (auxBuf[position] == (short) GlobalConfig.charsetLength - 1) {
        if (position == GlobalConfig.secretLength - 1) {
            for (size_t i = 0; i < GlobalConfig.secretLength; i++) {
                auxBuf[i] = 0;
            }
            return 0;
        }
        auxBuf[position] = 0;
        if (incSecretAuxBufBip39(auxBuf, position + 1, passBuf) == 0) {
            fillSecretBuffer(auxBuf, passBuf);
        }
        return 1;
    }
    auxBuf[position]++;
    if (position == 0) {
        char currChar;
        size_t bip_pc = 0;
        passBuf->offset += (*GlobalConfig.bipOffset)[auxBuf[position]];
        do {
            currChar = (*GlobalConfig.bipWords)[auxBuf[position]][bip_pc];
            passBuf->string[passBuf->offset + bip_pc] = currChar;
            bip_pc++;
        } while (currChar != 0);
        passBuf->string[passBuf->offset + bip_pc - 1] = ' ';
    }
    return 0;
}

/**
 * Converts values from auxBuf to a valid PASSPHRASE
 */
void fillSecretBuffer(short * auxBuf, struct PASSPHRASE * passBuf) {
    if (GlobalConfig.useBip39) {
        char temp[PASSPHRASE_MAX_LENGTH];
        size_t pc = 0;
        char currChar;
        for (size_t word = 0; word < GlobalConfig.secretLength; word++) {
            size_t bip_pc = 0;
            do {
                currChar = (*GlobalConfig.bipWords)[auxBuf[word]][bip_pc];
                temp[pc] = currChar;
                pc++;
                bip_pc++;
            } while (currChar != 0);
            temp[pc - 1] = ' ';
        }
        if (GlobalConfig.salt[0] == 0) {
            temp[pc - 1] = 0;
        } else {
            size_t salt_pc = 0;
            do {
                currChar = GlobalConfig.salt[salt_pc];
                temp[pc] = currChar;
                pc++;
                salt_pc++;
            } while (currChar != 0);
        }
        pc--;
        passBuf->offset = PASSPHRASE_MAX_LENGTH - pc;
        memcpy(passBuf->string + passBuf->offset, temp, pc);
    } else {
        passBuf->offset = PASSPHRASE_MAX_LENGTH - GlobalConfig.secretLength;
        for (size_t n = 0; n < GlobalConfig.secretLength; n++) {
            passBuf->string[passBuf->offset + n] = GlobalConfig.charset[auxBuf[n]];
        }
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

void checkAndPrintPassphraseStrength(void) {
    size_t i;
    char * foundChar;
    double passStrength;
    if (GlobalConfig.useBip39) {
        passStrength = log2(pow(2048.0, (double) GlobalConfig.secretLength)) + log2(pow(89.0, (double)strlen(GlobalConfig.salt)));
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
    if (passStrength < 128.0) {
        if (GlobalConfig.allowInsecure) {
            printf("Proceeding with weak passphrase: %.f bits strong\n", passStrength);
        } else {
            printf("Your passphrase would be %.f bits strong. Refusing to continue with a passphrase weaker than 128-bit. Increase pass-length, increase charset or use the option '--pir' if you are expert.\n\n", passStrength);
            exit(1);
        }
    } else if (passStrength < 256.0) {
        printf("Your passphrase will be %.f bits strong, if attacker knows details about the charset, length and salt.\n", passStrength);
    } else {
        printf("Good! Your passphrase will be stronger than the signum blockchain cryptography.\n");
    }
}

int main(int argc, char ** argv) {
    uint64_t * ID;
    struct PASSPHRASE * secretBuf;
    short * secretAuxBuf;
    char printMask[RS_ADDRESS_STRING_SIZE];
    char rsAddress[RS_ADDRESS_STRING_SIZE];
    char currentPassphrase[PASSPHRASE_MAX_LENGTH];
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

    maskIndex = argumentsParser(argc, argv);
    maskToByteMask(argv[maskIndex], GlobalConfig.mask, GlobalConfig.suffix);
    secretBuf = (struct PASSPHRASE *) malloc(sizeof (struct PASSPHRASE) * GlobalConfig.gpuThreads * 2);
    secretAuxBuf = (short *) malloc(sizeof (short) * GlobalConfig.gpuThreads * GlobalConfig.secretLength * 2);
    if (secretBuf == NULL || secretAuxBuf == NULL) {
        fprintf(stderr, "Cannot allocate memory for passphrases buffer\n");
        return EXIT_FAILURE;
    }
    initRand();
    seedRand(secretAuxBuf, GlobalConfig.gpuThreads * GlobalConfig.secretLength * 2);
    for (size_t n = 0; n < GlobalConfig.gpuThreads * 2; n++) {
        fillSecretBuffer(secretAuxBuf + n * GlobalConfig.secretLength, &secretBuf[n]);
    }
    checkAndPrintPassphraseStrength();
    if (GlobalConfig.useGpu) {
        ID = gpuInit();
    } else {
        ID = cpuInit();
    }
    if (GlobalConfig.appendDb || GlobalConfig.searchDb) {
        if (dbInit()) {
            return EXIT_FAILURE;
        }
    }
    byteMaskToPrintMask(GlobalConfig.mask, printMask);
    printf("Using mask %s\n", printMask);

    clock_gettime(CLOCK_REALTIME, &tstart);
#if MDEBUG == 0
    do {
        rounds++;
        short * currAuxBufItem = secretAuxBuf + (GlobalConfig.gpuThreads * GlobalConfig.secretLength) * ((rounds + 1) % 2);
        struct PASSPHRASE * currPassphrase = &secretBuf[GlobalConfig.gpuThreads * ((rounds + 1) % 2)];
        if (GlobalConfig.useBip39) {
            for (i = 0; i < GlobalConfig.gpuThreads; i++) {
                incSecretAuxBufBip39(currAuxBufItem, 0, currPassphrase);
                currAuxBufItem += GlobalConfig.secretLength;
                currPassphrase++;
            }
        } else {
            for (i = 0; i < GlobalConfig.gpuThreads; i++) {
                incSecretAuxBufRegular(currAuxBufItem, 0, currPassphrase);
                currAuxBufItem += GlobalConfig.secretLength;
                currPassphrase++;
            }
        }
#endif
        if (GlobalConfig.useGpu) {
            gpuSolver(&secretBuf[GlobalConfig.gpuThreads * ((rounds + 1) % 2)], ID);
        } else {
            cpuSolver(&secretBuf[GlobalConfig.gpuThreads * (rounds % 2)], ID);
        }
        if ((rounds % roundsToPrint) == 0) {
            clock_gettime(CLOCK_REALTIME, &tend);
            seconds = (tend.tv_sec - tstart.tv_sec);
            nanos = ((seconds * 1000000000LL) + tend.tv_nsec) - tstart.tv_nsec;
            timeInterval = (double) nanos / 1000000000.0;
            uint64_t currentTries = rounds * GlobalConfig.gpuThreads;
            printf(
                "\r %llu tries - %.0f tries/second..",
                PRINTF_CAST currentTries,
                (double) ((rounds - previousRounds) * GlobalConfig.gpuThreads) / timeInterval
            );
            fflush(stdout);
            clock_gettime(CLOCK_REALTIME, &tstart);
            // adjust rounds To print
            if (timeInterval < .7) {
                roundsToPrint *= 2;
            }
            if (timeInterval > 2) {
                roundsToPrint /= 2;
                if (roundsToPrint == 0) roundsToPrint++;
            }
            previousRounds = rounds;

            FILE * endFile = fopen("stop.txt", "r");
            if (endFile) {
                end = 1;
                fclose(endFile);
                fprintf(stdout, "\nStopped with 'stop.txt'\n");
            }
        }
        for (i = 0; i < GlobalConfig.gpuThreads; i++) {
            if (ID[i] != 0) {
                memcpy(
                    currentPassphrase,
                    secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].string + secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].offset,
                    PASSPHRASE_MAX_LENGTH - secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].offset
                );
                currentPassphrase[PASSPHRASE_MAX_LENGTH - secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].offset]='\0';
                if (GlobalConfig.appendDb == 1) {
                    if (dbInsert(ID[i], currentPassphrase)) {
                        end = 1;
                        fprintf(stdout, "\nIt seems we found a collision. Check 'collision.txt' for details.\n");
                        fclose(fopen("stop.txt", "a"));

                    }
                }
                if (GlobalConfig.searchDb == 1) {
                    if (dbSearch(ID[i], currentPassphrase)) {
                        end = 1;
                        fprintf(stdout, "\nIt seems we found a collision. Check 'collision.txt' for details.\n");
                        fclose(fopen("stop.txt", "a"));
                    }
                }
            }
        }
        
#if MDEBUG == 0
    } while (end == 0);
#else
    for (i = 0; i < GlobalConfig.gpuThreads; i++) {
        printf(
            "'%*.*s': %x\n",
            PASSPHRASE_MAX_LENGTH - (int)secretBuf[i].offset,
            PASSPHRASE_MAX_LENGTH - (int)secretBuf[i].offset,
            secretBuf[i].string + secretBuf[i].offset,
            ID[i]
        );
    }
    for (i = GlobalConfig.gpuThreads; i < GlobalConfig.gpuThreads * 2; i++) {
        printf(
            "'%*.*s': N/A\n",
            PASSPHRASE_MAX_LENGTH - (int)secretBuf[i].offset,
            PASSPHRASE_MAX_LENGTH - (int)secretBuf[i].offset,
            secretBuf[i].string + secretBuf[i].offset
        );
    }
#endif

    dbDestroy();
}
