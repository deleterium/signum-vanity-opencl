#include "globalTypes.h"
#include "dbHandler.h"

#include <mongoc/mongoc.h>
#include <bson.h>
#include <stdlib.h>
#include <string.h>

struct DB_DETAILS {
    mongoc_collection_t *collection;
    mongoc_uri_t *uri;
    mongoc_client_t *client;
    mongoc_database_t *database;
};

static struct DB_DETAILS dbDetails;
static bson_t *docs[INSERT_BATCH_SIZE];

static void init_all (bson_t **ptr) {
    for (int i = 0; i < INSERT_BATCH_SIZE; i++) {
        ptr[i] = bson_new ();
    }
}
static void destroy_all (bson_t **ptr) {
    for (int i = 0; i < INSERT_BATCH_SIZE; i++) {
        bson_destroy (ptr[i]);
    }
}

int dbInit(void) {
    const char *uri_string = CONNECTION_URI;
    bson_t *command, reply;
    bson_error_t error;
    char *str;
    bool retval;

    mongoc_init ();
    dbDetails.uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!dbDetails.uri) {
        fprintf (
            stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string,
            error.message);
        return EXIT_FAILURE;
    }
    dbDetails.client = mongoc_client_new_from_uri (dbDetails.uri);
    if (!dbDetails.client) {
        return EXIT_FAILURE;
    }
    mongoc_client_set_appname (dbDetails.client, CLIENT_NAME);
    dbDetails.database = mongoc_client_get_database (dbDetails.client, DATABASE_NAME);
    dbDetails.collection = mongoc_client_get_collection (dbDetails.client, DATABASE_NAME, COLLECTION_NAME);

    command = BCON_NEW ("ping", BCON_INT32 (1));
    retval = mongoc_client_command_simple ( dbDetails.client, "admin", command, NULL, &reply, &error);
    if (!retval) {
        fprintf (stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }

    init_all (docs);

    str = bson_as_json (&reply, NULL);
    fprintf (stdout, "%s\n", str);
    bson_free (str);
    bson_destroy(command);
    bson_destroy(&reply);

    return EXIT_SUCCESS;
}

void dbDestroy(void) {
    destroy_all (docs);
    mongoc_collection_destroy (dbDetails.collection);
    mongoc_database_destroy (dbDetails.database);
    mongoc_uri_destroy (dbDetails.uri);
    mongoc_client_destroy (dbDetails.client);
    mongoc_cleanup ();
}


int dbInsert(const uint64_t id, char * currPassphrase) {
    static int bsonCounter = 0;
    static bson_oid_t oid;
    static uint8_t newId[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    memcpy(newId, &id, sizeof(uint64_t));
    bson_reinit(docs[bsonCounter]);
    bson_oid_init_from_data(&oid, (uint8_t *) newId);
    bson_append_oid (docs[bsonCounter], "_id", 3, &oid);
    bson_append_utf8 (docs[bsonCounter], "passphrase", -1, currPassphrase, -1);

    if (++bsonCounter == INSERT_BATCH_SIZE) {
        bson_error_t error;
        bsonCounter = 0;
        if (! mongoc_collection_insert_many (dbDetails.collection,
                        (const bson_t **) docs,
                        (uint32_t) INSERT_BATCH_SIZE,
                        NULL,
                        NULL,
                        &error)) {
            // Add collision information at collision.txt
            FILE * foundFile = fopen("collision.txt", "a");
            fprintf (foundFile, "%s\n", error.message);
            for (int d=0; d<INSERT_BATCH_SIZE; d++) {
                char *j = bson_as_canonical_extended_json (docs[d], NULL);
                fprintf (foundFile, "%s\n", j);
                bson_free (j);
            }
            fclose(foundFile);
            // Create stop.txt to finish other instances
            fclose(fopen("stop.txt", "a"));
            // Alert user
            fprintf (stdout, "Found collision. Message:\n%s\n", error.message);
            return 1;
        }
    }

    return 0;
}