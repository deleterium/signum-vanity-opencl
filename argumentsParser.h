#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globalTypes.h"

extern struct CONFIG GlobalConfig;

char helpString[] = "\
Passphrase generator for vanity addresses on Signum cryptocurrency.\n\
\n\
Usage: vanity [OPTION [ARG]] ... MASK [OPTION [ARG]] ...\n\
  --help             Show this help statement\n\
  --suffix           Match given mask from the end of address. Default is to match from the beginning\n\
  --pass-length N    Passphrase length. Max 134 chars. Default: 64\n\
  --cpu              Set to use CPU and disable using GPU\n\
  --gpu              Set to use GPU. Already is default\n\
  --gpu-platform N   Select GPU from platorm N. Default: 0\n\
  --gpu-device N     Select GPU device N. Default: 0\n\
  --gpu-threads N    Send a batch of N threads. Default: 16384\n\
  --gpu-work-size N  Select N concurrent works. Default: 64\n\
  --endless          Never stop finding passphrases\n\
  --use-charset ABC  Generate passphrase only containing the ABC chars\n\
  --use-bip39        Generate passphrase with 12 words from BIP-39 list\n\
  --add-salt ABC     Add your salt to the bip39 word list\n\
  --append-db        Append (or create) database.csv with found results\n\
\n\
  MASK   Rules for the desired address. It must be at least one char long. No 0, O, I or 1 are allowed. Sometimes it is needed to embrace MASK with ' or \" chars. Following wildcars can be used:\n\
      ?: Matches any char\n\
      @: Matches only letters [A-Z]\n\
      #: Matches only numbers [2-9]\n\
      c: Matches consonants [BCDFGHJKLMNPQRSTVWXZ]\n\
      v: Matches vowels [AEUY]\n\
      -: Use to organize MASK, does not affect result\n\
\n\
Example: vanity --gpu --gpu-threads 102400 --gpu-work-size 32 V#N_@ --pass-length 40\n";

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
