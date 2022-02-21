#include "globalTypes.h"

/** Allocate memory for result buffer and prints a message */
uint64_t * cpuInit(void);

/**
 * Solves a batch of passphrases to ID's using the cpu to result buffer */
void cpuSolver(const struct PASSPHRASE * passphraseBatch, uint64_t * resultBatch);

/** Process calculation for only one passphrase, put the result in rsAddress and return
 * the account ID */
uint64_t solveOnlyOne(const struct PASSPHRASE * passphrase, char * rsAddress);
