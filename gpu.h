#include <stdio.h>
#include <stdlib.h>

#define CL_TARGET_OPENCL_VERSION 220

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE 0x10000000

void gpuInit(void);
void gpuSolver(char*, unsigned long *);
