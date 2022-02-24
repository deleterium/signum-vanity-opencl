#include "globalTypes.h"

#define INSERT_BATCH_SIZE 256
#define CONNECTION_URI "mongodb://localhost:27017"
#define CLIENT_NAME "vanityPRG"
#define DATABASE_NAME "vanityD"
#define COLLECTION_NAME "passphrases"

/**
 * Initializes database and print info to stdout and errors to stderr.
 * @returns 0 on SUCCESS or 1 if failure.
 */
int dbInit(void);

/**
 * Free some static allocated memory and call mongodb cleanup.
 */
void dbDestroy(void);

/**
 * Collect a record and insert at the collection in batches.
 * Save details to 'collision.txt' in case of insertion error, expected
 *  to be a collision.
 * @param id Calculated id.
 * @param passphrase Null terminated string.
 * @returns 0 on SUCCESS for insertion or 1 if error/collision.
 */
int dbInsert(const uint64_t id, const char * passphrase);

/**
 * Collect a record and search collection in batches.
 * Save details to 'collision.txt' in case of any document is found.
 * 
 * @param id Calculated id.
 * @param currPassphrase Null terminated string.
 * @return 0 if nothing found or 1 if collision
 */
int dbSearch(const uint64_t id, const char * currPassphrase);
