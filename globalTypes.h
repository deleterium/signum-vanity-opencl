#pragma once

#if defined(_WIN32)	|| defined(_WIN64) || defined(__TOS_WIN__) || defined(__WINDOWS__)
    #define OS_WINDOWS
#else
    #define OS_LINUX
#endif

/* compiler */
#if defined(_MSC_VER)
    #define COMPILER_MSVC
    #define PRINTF_CAST
#else
    #define COMPILER_GCC
    #define PRINTF_CAST (unsigned long long)
#endif

#if defined(COMPILER_MSVC)
    typedef signed char int8_t;
    typedef unsigned char uint8_t;
    typedef signed short int16_t;
    typedef unsigned short uint16_t;
    typedef signed int int32_t;
    typedef unsigned int uint32_t;
    typedef signed __int64 int64_t;
    typedef unsigned __int64 uint64_t;
#else
    #include <stdint.h>
#endif

#include "ReedSolomon.h"

struct CONFIG {
    uint64_t
        gpuThreads,
        gpuWorkSize,
        gpuPlatform,
        gpuDevice,
        secretLength;
    int32_t useGpu,
        suffix,
        endless;
    uint8_t mask[RS_ADDRESS_BYTE_SIZE];
    char charset[120];
};


