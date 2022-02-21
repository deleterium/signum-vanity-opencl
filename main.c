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

#include <libmongoc-1.0/mongoc/mongoc.h>
#include <libbson-1.0/bson.h>

// Compiling command:
// gcc -o vanity main.c cpu.c gpu.c ReedSolomon.c ed25519-donna/ed25519.c argumentsParser.c -m64 -lcrypto  -lOpenCL -lm -O2 -Wextra $(pkg-config --libs --cflags libmongoc-1.0)

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

static void destroy_all (bson_t **ptr, int n) {
    for (int i = 0; i < n; i++) {
        bson_destroy (ptr[i]);
    }
}

#define N_BSONS 256

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

    /* START MONGO INITIALIZATION */
    const char *uri_string = "mongodb://localhost:27017";
    mongoc_uri_t *uri;
    mongoc_client_t *client;
    mongoc_database_t *database;
    mongoc_collection_t *collection;
    bson_t *command, reply, *insert;
    bson_error_t error;
    char *str;
    bool retval;
    /*
    * Required to initialize libmongoc's internals
    */
    mongoc_init ();
    /*
    * Safely create a MongoDB URI object from the given string
    */
    uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!uri) {
        fprintf (
            stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string,
            error.message);
        return EXIT_FAILURE;
    }
    /*
    * Create a new client instance
    */
    client = mongoc_client_new_from_uri (uri);
    if (!client) {
        return EXIT_FAILURE;
    }
    /*
    * Register the application name so we can track it in the profile logs
    * on the server. This can also be done from the URI (see other examples).
    */
    mongoc_client_set_appname (client, "vanityPRG");
    /*
    * Get a handle on the database "db_name" and collection "coll_name"
    */
    database = mongoc_client_get_database (client, "vanity");
    collection = mongoc_client_get_collection (client, "vanity", "passphrases");
    /*
    * Do work. This example pings the database, prints the result as JSON and
    * performs an insert
    */
    command = BCON_NEW ("ping", BCON_INT32 (1));
    retval = mongoc_client_command_simple ( client, "admin", command, NULL, &reply, &error);
    if (!retval) {
        fprintf (stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }
    str = bson_as_json (&reply, NULL);
    printf ("%s\n", str);
    bson_free (str);
    /* END MONGO INITIALIZATION */

    maskIndex = argumentsParser(argc, argv);
    maskToByteMask(argv[maskIndex], GlobalConfig.mask, GlobalConfig.suffix);
    secretBuf = (struct PASSPHRASE *) malloc(sizeof (struct PASSPHRASE) * GlobalConfig.gpuThreads * 2);
    secretAuxBuf = (short *) malloc(sizeof (short) * GlobalConfig.gpuThreads * GlobalConfig.secretLength * 2);
    if (secretBuf == NULL || secretAuxBuf == NULL) {
        printf("Cannot allocate memory for passphrases buffer\n");
        exit(1);
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

    bson_t *docs[N_BSONS];
    bson_oid_t oid;
    int bsonCounter = 0;
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
                printf("\n");
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
                uint64_t newId[2];
                newId[0] = ID[i];
                newId[1] = 0;
                if (GlobalConfig.appendDb == 1) {
                    docs[bsonCounter] = bson_new ();
                    bson_oid_init_from_data(&oid, (uint8_t *) newId);
                    bson_append_oid (docs[bsonCounter], "_id", 3, &oid);
                    bson_append_utf8 (docs[bsonCounter], "passphrase", -1, currentPassphrase, -1);
                    if (++bsonCounter == 256) {
                        retval = mongoc_collection_insert_many (collection,
                                        (const bson_t **) docs,
                                        (uint32_t) N_BSONS,
                                        NULL,
                                        NULL,
                                        &error);
                        if (!retval) {
                            FILE * foundFile = fopen("collision.txt", "a");
                            fprintf (foundFile, "%s\n", error.message);
                            for (int d=0; d<N_BSONS; d++) {
                                char *j = bson_as_canonical_extended_json (docs[d], NULL);
                                fprintf (foundFile, "%s\n", j);
                                bson_free (j);
                            }
                            fclose(foundFile);
                            foundFile = fopen("stop.txt", "a");
                            fclose(foundFile);
                            fprintf (stderr, "Found collision. Message:\n%s\n", error.message);
                            end = 1;
                            break;
                        }
                        destroy_all (docs, N_BSONS);
                        bsonCounter = 0;
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

    /*
     * Release our handles and clean up libmongoc
     */
    mongoc_collection_destroy (collection);
    mongoc_database_destroy (database);
    mongoc_uri_destroy (uri);
    mongoc_client_destroy (client);
    mongoc_cleanup ();
}
