
#ifndef CONFIG_VARS
#define CONFIG_VARS

struct CONFIG {
    unsigned long
        useGpu,
        gpuThreads,
        gpuWorkSize,
        gpuPlatform,
        gpuDevice;
    int secretLength;

};

#endif
