#include <stdio.h>
#include <stdlib.h>

#define CL_TARGET_OPENCL_VERSION 120

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE 0x10000000

/** Initializes the GPU and returns a pointer for the allocate memory for resultBuffer */
uint64_t * gpuInit(void);

/** Solve a batch of passphrases and set result to result buffer */
void gpuSolver(struct PASSPHRASE *, uint64_t *);
