
typedef unsigned char BYTE;

/** Initializes cpu (actually just prints a message) */
void cpuInit(void);

/**
 * Transforms a batch of passphrases to ID's using the cpu */
void cpuSolver(const char* passphraseBatch, unsigned char * resultBatch);