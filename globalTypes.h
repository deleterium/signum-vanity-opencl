#ifndef CONFIG_VARS
#define CONFIG_VARS

#include "ReedSolomon.h"

struct CONFIG {
    unsigned long
        useGpu,
        gpuThreads,
        gpuWorkSize,
        gpuPlatform,
        gpuDevice;
    int secretLength,
        endless;
    BYTE mask[RS_ADDRESS_BYTE_SIZE];
};

#endif
