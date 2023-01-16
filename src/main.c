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

void loadBipFile(void) {
    FILE * fp;
    fp = fopen(GlobalConfig.bipFilename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load bip words file '%s'.\n", GlobalConfig.bipFilename);
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    size_t source_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char * bipSource = (char*)malloc(source_size * sizeof(char));
    if (fread(bipSource, sizeof(char), source_size, fp) != source_size) {
        fprintf(stderr, "Failed to read bip words file '%s'.\n", GlobalConfig.bipFilename);
        exit(1);
    }
    fclose(fp);

    size_t numOfRecords = 0;
    for (size_t i = 0; i < source_size; i++) {
        if (bipSource[i] == '\n') {
            numOfRecords++;
        }
    }
    GlobalConfig.bipWords = malloc(numOfRecords * BIPWORD_BUFFER_SIZE * sizeof(char));
    GlobalConfig.bipOffset = malloc(numOfRecords * sizeof(char));
    GlobalConfig.charsetLength = numOfRecords;

    int lastRecSize = 0;
    int pcBip = 0;
    for (size_t i = 0, rec = 0; i < source_size; i++) {
        switch (bipSource[i]) {
        case '\n':
            (*GlobalConfig.bipWords)[rec][pcBip] = 0;
            (*GlobalConfig.bipOffset)[rec] = lastRecSize - pcBip;
            rec++;
            lastRecSize = pcBip;
            pcBip = 0;
            continue;
        case '\r':
            continue;
        default:
            if (pcBip == BIPWORD_BUFFER_SIZE - 1) {
                fprintf(stderr, "Buffer overflow reading bip words file '%s'.\n", GlobalConfig.bipFilename);
                exit(1);
            }
           (*GlobalConfig.bipWords)[rec][pcBip] = bipSource[i];
           pcBip++;
        }
    }
    if (pcBip != 0) {
        fprintf(stderr, "Error reading bip words file '%s'. Missing new line at the end.\n", GlobalConfig.bipFilename);
        exit(1);
    }
    free(bipSource);
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
        char temp[2 * PASSPHRASE_MAX_LENGTH];
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
        if (pc > PASSPHRASE_MAX_LENGTH - BIPWORD_BUFFER_SIZE) {
            // Overflow, remove last word
            pc = PASSPHRASE_MAX_LENGTH - BIPWORD_BUFFER_SIZE;
            while (temp[pc] != ' ') {
                --pc;
            }
        }
        passBuf->offset = (uint8_t)(PASSPHRASE_MAX_LENGTH - pc);
        memcpy(passBuf->string + passBuf->offset, temp, pc);
    } else {
        passBuf->offset = (uint8_t)(PASSPHRASE_MAX_LENGTH - GlobalConfig.secretLength);
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
        passStrength = log2(pow((double) GlobalConfig.charsetLength, (double) GlobalConfig.secretLength)) + log2(pow(89.0, (double)strlen(GlobalConfig.salt)));
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

double estimate90percent(double findingChance) {
    return (-1.0 / log10((1.0 - findingChance)));
}

double luckyChance(double numberOfEvents, double findingChance) {
    return (1.0 - pow(1.0 - findingChance,numberOfEvents)) * 100.0;
}

void appendDb(char *passphrase, char *address, uint64_t id, char *publicKey, char * extendedPK) {
    FILE *database = fopen("database.csv", "a");
    if (database == NULL) {
        printf("Failed operation to open database file.\n");
        exit(1);
    }
    fprintf(
        database,
        "\"%s\",\"%s-%s\",\"%llu\",\"%s\"\n",
        passphrase,
        address,
        extendedPK,
        PRINTF_CAST id,
        publicKey
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
    char publicKeyHex[PK_HEX_LENGTH];
    char extendedPublicKey[EXTENDED_PK_LENGTH];
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

#ifdef OS_WINDOWS
    // Allow utf-8 strings
    SetConsoleOutputCP(65001);
#endif

    maskIndex = argumentsParser(argc, argv);
    maskToByteMask(argv[maskIndex], GlobalConfig.mask, GlobalConfig.suffix);
    secretBuf = (struct PASSPHRASE *) malloc(sizeof (struct PASSPHRASE) * GlobalConfig.gpuThreads * 2);
    secretAuxBuf = (short *) malloc(sizeof (short) * GlobalConfig.gpuThreads * GlobalConfig.secretLength * 2);
    if (secretBuf == NULL || secretAuxBuf == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
    }
    if (GlobalConfig.useBip39) {
        loadBipFile();
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
    byteMaskToPrintMask(GlobalConfig.mask, printMask);
    printf("Using mask %s\n", printMask);
    eventChance = findingChance(GlobalConfig.mask);
    printf(" %.0f tries for 90%% chance finding a match. Ctrl + C to cancel.\n", estimate90percent(eventChance));

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
            solveOnlyOne(&secretBuf[GlobalConfig.gpuThreads * (rounds % 2)], rsAddress, NULL, NULL);
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
                memcpy(
                    currentPassphrase,
                    secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].string + secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].offset,
                    PASSPHRASE_MAX_LENGTH - secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].offset
                );
                currentPassphrase[PASSPHRASE_MAX_LENGTH - secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i].offset]='\0';
                uint64_t newId = solveOnlyOne(&secretBuf[GlobalConfig.gpuThreads * (rounds % 2) + i], rsAddress, publicKeyHex, extendedPublicKey);
                if (GlobalConfig.appendDb == 1) {
                    appendDb(currentPassphrase, rsAddress, newId, publicKeyHex, extendedPublicKey);
                }
                if (GlobalConfig.endless == 0) {
                    printf(
                        "\nID: %20llu RS: %s-%s Passphrase: '%s'\n",
                        PRINTF_CAST newId,
                        rsAddress,
                        extendedPublicKey,                       
                        currentPassphrase
                    );
                    fflush(stdout);
                    end = 1;
                    break;
                } else {
                    printf(
                        "\rID: %20llu RS: %s-%s Passphrase: '%s'\n",
                        PRINTF_CAST newId,
                        rsAddress,
                        extendedPublicKey,
                        currentPassphrase
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
}
