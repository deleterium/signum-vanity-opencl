
typedef unsigned char BYTE;

/** Allocate memory for result buffer and prints a message */
unsigned char * cpuInit(void);

/**
 * Solves a batch of passphrases to ID's using the cpu to result buffer */
void cpuSolver(const char* passphraseBatch, unsigned char * resultBatch);

/** Process calculation for only one passphrase, put the result in rsAddress and return
 * the account ID */
unsigned long solveOnlyOne(const char* passphrase, char* rsAddress);
