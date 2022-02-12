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
extern char bipWords[2048][9];

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
void incSecretAuxBuf(short * auxBuf, size_t position, struct PASSPHRASE * passBuf) {
    if (auxBuf[position] == (short) GlobalConfig.charsetLength - 1) {
        if (position == GlobalConfig.secretLength - 1) {
            for (size_t i = 0; i < GlobalConfig.secretLength; i++) {
                auxBuf[i] = 0;
                if (GlobalConfig.useBip39 == 0){
                    passBuf->string[i] = GlobalConfig.charset[0];
                }
            }
            return;
        }
        auxBuf[position] = 0;
        if (GlobalConfig.useBip39 == 0){
            passBuf->string[position] = GlobalConfig.charset[0];
        }
        incSecretAuxBuf(auxBuf, position + 1, passBuf);
        return;
    }
    auxBuf[position]++;
    if (GlobalConfig.useBip39 == 0){
        passBuf->string[position] = GlobalConfig.charset[auxBuf[position]];
    } else {
        if (position == 0) {
            size_t pc = 0;
            for (size_t word=0; word<12; word++) {
                size_t bip_pc = 0;
                while (bipWords[auxBuf[word]][bip_pc] != 0) {
                    passBuf->string[pc] = bipWords[auxBuf[word]][bip_pc];
                    pc++;
                    bip_pc++;
                }
                passBuf->string[pc] = ' ';
                pc++;
            }
            if (GlobalConfig.salt[0] == 0) {
                pc--;
                passBuf->string[pc] = 0;
                passBuf->length = (uint8_t)pc;
            } else {
                pc++;
                size_t salt_pc = 0;
                while (GlobalConfig.salt[salt_pc] != 0) {
                    passBuf->string[pc] = GlobalConfig.salt[salt_pc];
                    pc++;
                    salt_pc++;
                }
                passBuf->string[pc] = 0;
            }
        }
    }
}

/**
 * Converts values from auxBuf to a valid PASSPHRASE
 */
void fillSecretBuffer(short * auxBuf, struct PASSPHRASE * passBuf) {
    if (GlobalConfig.useBip39) {
        if (GlobalConfig.salt[0] == 0) {
            passBuf->length = (uint8_t)sprintf(passBuf->string, "%s %s %s %s %s %s %s %s %s %s %s %s",
                bipWords[auxBuf[0]],
                bipWords[auxBuf[1]],
                bipWords[auxBuf[2]],
                bipWords[auxBuf[3]],
                bipWords[auxBuf[4]],
                bipWords[auxBuf[5]],
                bipWords[auxBuf[6]],
                bipWords[auxBuf[7]],
                bipWords[auxBuf[8]],
                bipWords[auxBuf[9]],
                bipWords[auxBuf[10]],
                bipWords[auxBuf[11]]
            );
        } else {
            passBuf->length = (uint8_t)sprintf(passBuf->string, "%s %s %s %s %s %s %s %s %s %s %s %s %s",
                bipWords[auxBuf[0]],
                bipWords[auxBuf[1]],
                bipWords[auxBuf[2]],
                bipWords[auxBuf[3]],
                bipWords[auxBuf[4]],
                bipWords[auxBuf[5]],
                bipWords[auxBuf[6]],
                bipWords[auxBuf[7]],
                bipWords[auxBuf[8]],
                bipWords[auxBuf[9]],
                bipWords[auxBuf[10]],
                bipWords[auxBuf[11]],
                GlobalConfig.salt
            );
        }
    } else {
        for (size_t n = 0; n < GlobalConfig.secretLength; n++) {
            passBuf->string[n] = GlobalConfig.charset[auxBuf[n]];
        }
        passBuf->length = (uint8_t)GlobalConfig.secretLength;
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
        passStrength = log2(pow(2048.0, 12.0)) + log2(pow(89.0, (double)strlen(GlobalConfig.salt)));
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
        printf("Your passphrase would be %.f bits strong. Refusing to continue with a passphrase weaker than 128-bit. Increase pass-length or increase charset length.\n\n", passStrength);
        exit(1);
    } else if (passStrength < 256.0) {
        printf("Your passphrase will be %.f bits strong, if attacker knows details about the charset, length and salt.\n", passStrength);
    } else {
        printf("Good! Your passphrase will be stronger than the signum blockchain cryptography.\n");
    }
}

double estimate90percent(double findingChance) {
    return (-1.0 / log10((1.0 - findingChance)));
}

double luckyChance(double numberOfEvents, double findingChance) {
    return (1.0 - pow(1.0 - findingChance,numberOfEvents)) * 100.0;
}

void appendDb(char *passphrase, char *address, uint64_t id) {
    FILE *database = fopen("database.csv", "a");
    if (database == NULL) {
        printf("Failed operation to open database file.\n");
        exit(1);
    }
    fprintf(
        database,
        "\"%s\",\"%s\",\"%llu\"\n",
        passphrase,
        address,
        PRINTF_CAST id
    );
    fclose(database);
}

int main(int argc, char ** argv) {
    uint8_t * ID;
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

    initRand();
    maskIndex = argumentsParser(argc, argv);
    maskToByteMask(argv[maskIndex], GlobalConfig.mask, GlobalConfig.suffix);
    secretBuf = (struct PASSPHRASE *) malloc(sizeof (struct PASSPHRASE) * GlobalConfig.gpuThreads * 2);
    secretAuxBuf = (short *) malloc(sizeof (short) * GlobalConfig.gpuThreads * GlobalConfig.secretLength * 2);
    if (secretBuf == NULL || secretAuxBuf == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
    }
    seedRand(secretAuxBuf, GlobalConfig.gpuThreads * GlobalConfig.secretLength * 2);
    for (size_t n = 0; n < GlobalConfig.gpuThreads * 2; n++) {
        fillSecretBuffer(secretAuxBuf + n * GlobalConfig.secretLength, &secretBuf[n]);
    }
    if (GlobalConfig.useGpu) {
        ID = gpuInit();
    } else {
        ID = cpuInit();
    }

    byteMaskToPrintMask(GlobalConfig.mask, printMask);
    printf("Using MASK %s\n", printMask);
    checkAndPrintPassphraseStrength();
    eventChance = findingChance(GlobalConfig.mask);
    printf(" %.0f tries for 90%% chance finding a match. Ctrl + C to cancel.\n", estimate90percent(eventChance));

    clock_gettime(CLOCK_REALTIME, &tstart);
#if MDEBUG == 0
    do {
        rounds++;
        for (i = 0; i < GlobalConfig.gpuThreads; i++) {
            short * currAuxBufItem = secretAuxBuf + (GlobalConfig.gpuThreads * GlobalConfig.secretLength) * ((rounds + 1) % 2) + GlobalConfig.secretLength * i;
            size_t currSecret = GlobalConfig.gpuThreads * ((rounds + 1) % 2) + i;
            incSecretAuxBuf(currAuxBufItem, 0, &secretBuf[currSecret]);
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
            solveOnlyOne(&secretBuf[GlobalConfig.gpuThreads * (rounds % 2)], rsAddress);
            printf(
                "\r %llu tries - Lucky chance: %.1f%% - %.0f tries/second. Last generated S-%s",
                PRINTF_CAST currentTries,
                luckyChance((double)currentTries, eventChance),
                (double) ((rounds - previousRounds) * GlobalConfig.gpuThreads) / timeInterval,
                rsAddress
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
        }
        for (i = 0; i < GlobalConfig.gpuThreads; i++) {
            if (ID[i] == 1) {
                memcpy(currentPassphrase, secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].string, secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].length);
                currentPassphrase[secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].length]='\0';
                uint64_t newId = solveOnlyOne(&secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i], rsAddress);
                if (GlobalConfig.appendDb == 1) {
                    appendDb(currentPassphrase, rsAddress, newId);
                }
                if (GlobalConfig.endless == 0) {
                    printf(
                        "\nPassphrase: '%s' RS: S-%s id: %20llu\n",
                        currentPassphrase,
                        rsAddress,
                        PRINTF_CAST newId
                    );
                    fflush(stdout);
                    end = 1;
                    break;
                } else {
                    printf(
                        "\rPassphrase: '%s' RS: S-%s id: %20llu\n",
                        currentPassphrase,
                        rsAddress,
                        PRINTF_CAST newId
                    );
                    fflush(stdout);
                    rounds %= 2;
                    previousRounds = rounds;
                }
            }
        }
#if MDEBUG == 0
    } while (end == 0);
#else
    for (i = 0; i < GlobalConfig.gpuThreads; i++) {
        printf(
            "'%*.*s': %x\n",
            (int)secretBuf[i].length,
            (int)secretBuf[i].length,
            secretBuf[i].string,
            ID[i]
        );
    }
    for (i = GlobalConfig.gpuThreads; i < GlobalConfig.gpuThreads * 2; i++) {
        printf(
            "'%*.*s': N/A\n",
            (int)secretBuf[i].length,
            (int)secretBuf[i].length,
            secretBuf[i].string
        );
    }
#endif
}
