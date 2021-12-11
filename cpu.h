
/** Allocate memory for result buffer and prints a message */
uint8_t * cpuInit(void);

/**
 * Solves a batch of passphrases to ID's using the cpu to result buffer */
void cpuSolver(const char * passphraseBatch, unsigned char * resultBatch);

/** Process calculation for only one passphrase, put the result in rsAddress and return
 * the account ID */
uint64_t solveOnlyOne(const char * passphrase, char * rsAddress);
