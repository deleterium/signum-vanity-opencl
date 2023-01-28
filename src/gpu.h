#include <stdio.h>
#include <stdlib.h>

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/** Initializes the GPU and returns a pointer for the allocate memory for resultBuffer */
uint8_t * gpuInit(void);

/** Solve a batch of passphrases and set result to result buffer */
void gpuSolver(struct PASSPHRASE *, uint8_t *);
