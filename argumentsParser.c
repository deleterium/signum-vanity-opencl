#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globalTypes.h"

#include "argumentsParser.h"
#include "bip39WordsEN.h"
#include "bip39WordsPT.h"
#include "bip39WordsES.h"

extern struct CONFIG GlobalConfig;
extern const char bipWordsEN[2048][16];
extern const char bipWordsPT[2048][16];
extern const char bipWordsES[2048][16];
extern const char bipOffsetEN[2048];
extern const char bipOffsetPT[2048];
extern const char bipOffsetES[2048];

const char helpString[] = R"(Passphrase generator for vanity addresses on Signum cryptocurrency.

Usage: vanity [OPTION]... MASK
Example: vanity --cpu --pass-length 32 SGN@

Options:
  --help             Show this help statement
  --suffix           Match given mask from the end of address. Default is to match from the beginning
  --pass-length N    Passphrase length. Max 134 chars. Default: 64
  --cpu              Set to use CPU and disable using GPU
  --gpu              Set to use GPU. Already is default
  --gpu-platform N   Select GPU from platorm N. Default: 0
  --gpu-device N     Select GPU device N. Default: 0
  --gpu-threads N    Send a batch of N threads. Default: 16384
  --gpu-work-size N  Select N concurrent works. Default: 64
  --endless          Never stop finding passphrases
  --use-charset ABC  Generate passphrase only containing the ABC chars
  --use-bip39        Generate passphrase with 12 words from BIP-39 list
  --dict [EN|PT|ES]  Dictionary language for bip-39. Default is english
  --add-salt ABC     Add your salt to the bip39 word list
  --append-db        Append (or create) database.csv with found results

Mask:
  Specify the rules for the desired address.
  It must be at least one char long.
  No 0, O, I or 1 are allowed.
  Following wildcars can be used:
    ?: Matches any char
    @: Matches only letters [A-Z]
    #: Matches only numbers [2-9]
    c: Matches consonants [BCDFGHJKLMNPQRSTVWXZ]
    v: Matches vowels [AEUY]
    p: Matches previous char
    -: Use to organize the mask, does not affect result
)";

void endProgram(const char * errorString) {
    printf("%s\n", errorString);
    exit(1);
}

int argumentsParser(int argc, char **argv) {
    int maskIndex = -1;
    //Default values:
    GlobalConfig.secretLength = 64;
    GlobalConfig.useGpu = 1;
    GlobalConfig.gpuThreads = 128 * 128;
    GlobalConfig.gpuWorkSize = 64;
    GlobalConfig.gpuPlatform = 0;
    GlobalConfig.gpuDevice = 0;
    GlobalConfig.endless = 0;
    GlobalConfig.suffix = 0;
    strcpy(GlobalConfig.charset, "!#$%&()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_abcdefghijklmnopqrstuvwxyz");
    GlobalConfig.charsetLength = strlen(GlobalConfig.charset);
    GlobalConfig.salt[0] = 0;
    GlobalConfig.appendDb = 0;
    GlobalConfig.useBip39 = 0;
    GlobalConfig.bipWords = &bipWordsEN;
    GlobalConfig.bipOffset = &bipOffsetEN;


    if (argc == 1) {
            endProgram("Usage: vanity [OPTION [ARG]] ... MASK\nTry '--help'.");
    }
    int i = 0;
    while (i < argc - 1) {
        i++;
        if (strlen(argv[i]) <= 2) {
            if (maskIndex == -1) {
                maskIndex = i;
                continue;
            } else {
                endProgram("Usage: vanity [OPTION [ARG]] ... MASK\nTry '--help'.");
            }
        }
        if (argv[i][0] != '-' || argv[i][1] != '-') {
            if (maskIndex == -1) {
                maskIndex = i;
                continue;
            } else {
                endProgram("Usage: vanity [OPTION [ARG]] ... MASK\nTry '--help'.");
            }
        }
        if (strcmp(argv[i], "--pass-length") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for pass-length.");
            }
            if (GlobalConfig.useBip39) {
                endProgram("Option --pass-length conflicts with --use-bip39.");
            }
            GlobalConfig.secretLength = strtol(argv[i], NULL, 10);
            if (GlobalConfig.secretLength >= PASSPHRASE_MAX_LENGTH){
                endProgram("Passphrase length exceeded maximum value.\nTry '--help'.");
            }
            continue;
        }
        if (strcmp(argv[i], "--gpu-threads") == 0) {
            i++;
            if (i >= argc){
                endProgram("Expecting value for gpu-threads.");
            }
            GlobalConfig.gpuThreads = strtol(argv[i], NULL, 10);
            if (GlobalConfig.gpuThreads == 0) {
                endProgram("Invalid value for gpu-threads.");
            }
            continue;
        }
        if (strcmp(argv[i], "--gpu-work-size") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for gpu-work-size.");
            }
            GlobalConfig.gpuWorkSize = strtol(argv[i], NULL, 10);
            if (GlobalConfig.gpuWorkSize == 0) {
                endProgram("Invalid value for gpu-work-size.");
            }
            continue;
        }
        if (strcmp(argv[i], "--gpu-platform") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for gpu-platform.");
            }
            GlobalConfig.gpuPlatform = strtol(argv[i], NULL, 10);
            if (GlobalConfig.gpuPlatform == 0 && argv[i][0] != '0') {
                endProgram("Invalid value for gpu-platform.");
            }
            continue;
        }
        if (strcmp(argv[i], "--gpu-device") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for gpu-device.");
            }
            GlobalConfig.gpuDevice = strtol(argv[i], NULL, 10);
            if (GlobalConfig.gpuDevice == 0 && argv[i][0] != '0') {
                endProgram("Invalid value for gpu-device.");
            }
            continue;
        }
        if (strcmp(argv[i], "--use-charset") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for use-charset.");
            }
            if (strlen(argv[i]) > 119) {
                endProgram("Charset values must be max 119 chars long.");
            }
            if (GlobalConfig.useBip39) {
                endProgram("Option --use-charset conflicts with --use-bip39.");
            }
            strcpy(GlobalConfig.charset, argv[i]);
            GlobalConfig.charsetLength = strlen(GlobalConfig.charset);
            continue;
        }
        if (strcmp(argv[i], "--add-salt") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for salt.");
            }
            if (strlen(argv[i]) >= PASSPHRASE_MAX_LENGTH - BIP_MAX_LENGTH) {
                endProgram("Salt is too long.");
            }
            strcpy(GlobalConfig.salt, argv[i]);
            for (size_t j = 0; j < strlen(GlobalConfig.salt); j++) {
                if (GlobalConfig.salt[j] < 32 || GlobalConfig.salt[j] > 126) {
                    endProgram("No special (unicode) chars are allowed in salt.\n");
                }
            }
            continue;
        }
        if (strcmp(argv[i], "--dict") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for language.");
            }
            if (strcmp(argv[i], "EN") == 0) {
                GlobalConfig.bipWords = &bipWordsEN;
                GlobalConfig.bipOffset = &bipOffsetEN;
                continue;
            }
            if (strcmp(argv[i], "PT") == 0) {
                GlobalConfig.bipWords = &bipWordsPT;
                GlobalConfig.bipOffset = &bipOffsetPT;
                continue;
            }
            if (strcmp(argv[i], "ES") == 0) {
                GlobalConfig.bipWords = &bipWordsES;
                GlobalConfig.bipOffset = &bipOffsetES;
                continue;
            }
            endProgram("Unknown language for dictionary.");
        }
        if (strcmp(argv[i], "--help") == 0) {
            printf("%s\n", helpString);
            exit(0);
        }
        if (strcmp(argv[i], "--gpu") == 0) {
            GlobalConfig.useGpu = 1;
            continue;
        }
        if (strcmp(argv[i], "--cpu") == 0) {
            GlobalConfig.useGpu = 0;
            GlobalConfig.gpuThreads = 256;
            continue;
        }
        if (strcmp(argv[i], "--use-bip39") == 0) {
            GlobalConfig.useBip39 = 1;
            GlobalConfig.secretLength = 12;
            GlobalConfig.charsetLength = 2048;
            continue;
        }
        if (strcmp(argv[i], "--endless") == 0) {
            GlobalConfig.endless = 1;
            continue;
        }
        if (strcmp(argv[i], "--suffix") == 0) {
            GlobalConfig.suffix = 1;
            continue;
        }
        if (strcmp(argv[i], "--append-db") == 0) {
            GlobalConfig.appendDb = 1;
            continue;
        }

        printf("Unknow command line option: %s\nTry '--help'.\n", argv[i]);
        exit(1);
    }
    if (maskIndex == -1) {
        endProgram("Error: MASK was not specified... Try '--help'.");
    }
    return maskIndex;
}
