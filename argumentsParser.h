#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globalTypes.h"

extern struct CONFIG GlobalConfig;

char helpString[] = "\
Password generator for vanity addresses on signum cryptocurrency.\n\
\n\
Usage: vanity [OPTION [ARG]] ... MASK\n\
  --help             show this help statement\n\
  --pass-length N    Passphrase length. 40 to 120 chars. Default: 64\n\
  --cpu              Set to use CPU. Using it disables using GPU.\n\
  --gpu              Set to use GPU. Default is to use.\n\
  --gpu-platform N   Select GPU from platorm N\n\
  --gpu-device N     Select GPU device N\n\
  --gpu-threads N    Send a batch of N threads\n\
  --gpu-work-size N  Select N concurrent works\n\
  --endless          Never stop finding passphrases\n\
\n\
  MASK   Desired address. Use '_' for any char at that location. Must be at least one char and maximum 17 chars. No 0, O, I or 1 allowed.\n\
\n\
Example: vanity --gpu --gpu-threads 102400 --gpu-work-size 32 V_A_N_I\n";

void endProgram(char * errorString) {
    printf("%s\n", errorString);
    exit(1);
}

void argumentsParser(int argc, char **argv) {

    //Default values:
    GlobalConfig.secretLength = 64;
    GlobalConfig.useGpu = 1;
    GlobalConfig.gpuThreads = 128*128;
    GlobalConfig.gpuWorkSize = 64;
    GlobalConfig.gpuPlatform = 0;
    GlobalConfig.gpuDevice = 0;
    GlobalConfig.endless = 0;

    if (argc == 1) {
            endProgram("Error: MASK was not specified... Try '--help'. ");
    }

    int i = 1;
    while (i < argc) {
        if (strlen(argv[i]) <=2 ) {
            if (i == argc -1) {
                // Last item is MASK
                return;
            }
            endProgram("Error parsing command line options. Try '--help'. ");
        }
        if (argv[i][0] != '-' || argv[i][1] != '-') {
            if (i == argc -1) {
                // Last item is MASK
                return;
            }
            endProgram("Error parsing command line options. Try '--help'. ");
        }

        if (strcmp(argv[i], "--pass-length") == 0) {
            i++;
            if (i >= argc){
                endProgram("Expecting value for pass-length.");
            }
            GlobalConfig.secretLength = (int) strtol(argv[i], NULL, 10);
            if (GlobalConfig.secretLength < 40 || GlobalConfig.secretLength > 120){
                endProgram("Invalid value for pass-length.");
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "--gpu-threads") == 0) {
            i++;
            if (i >= argc){
                endProgram("Expecting value for gpu-threads.");
            }
            GlobalConfig.gpuThreads = strtol (argv[i], NULL, 10);
            if (GlobalConfig.gpuThreads == 0){
                endProgram("Invalid value for gpu-threads.");
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "--gpu-work-size") == 0) {
            i++;
            if (i >= argc){
                endProgram("Expecting value for gpu-work-size.");
            }
            GlobalConfig.gpuWorkSize = strtol (argv[i], NULL, 10);
            if (GlobalConfig.gpuWorkSize == 0){
                endProgram("Invalid value for gpu-work-size.");
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "--gpu-platform") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for gpu-platform.");
            }
            GlobalConfig.gpuPlatform = strtol (argv[i], NULL, 10);
            if (GlobalConfig.gpuPlatform == 0 && argv[i][0] != '0' ){
                endProgram("Invalid value for gpu-platform.");
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "--gpu-device") == 0) {
            i++;
            if (i >= argc) {
                endProgram("Expecting value for gpu-device.");
            }
            GlobalConfig.gpuDevice = strtol (argv[i], NULL, 10);
            if (GlobalConfig.gpuDevice == 0 && argv[i][0] != '0' ){
                endProgram("Invalid value for gpu-device.");
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "--help") == 0) {
            printf("%s\n", helpString);
            exit(0);
        }
        if (strcmp(argv[i], "--gpu") == 0) {
            GlobalConfig.useGpu = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "--cpu") == 0) {
            GlobalConfig.useGpu = 0;
            GlobalConfig.gpuThreads = 256;
            i++;
            continue;
        }
        if (strcmp(argv[i], "--endless") == 0) {
            GlobalConfig.endless = 1;
            i++;
            continue;
        }

        printf("Unknow command line option: %s\nTry '--help'.\n", argv[i]);
        exit(1);
    }
    endProgram("Error: MASK was not specified... Try '--help'.");
}
