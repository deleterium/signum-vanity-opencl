#include "programConstants.h"
#include "sha256.h"
#include <time.h>
#include <sys/time.h>

void randstring(char * buffer, size_t length) {
    if (length) {
        int n;
        for (n = 0;n < length;n++) {
            char key = rand() % 89;
            buffer[n] = '!' + key;
        }
    }
}

void incSecret(char * secret, size_t position) {
    if (secret[position] >= 'z') {
        secret[position] = '!';
        incSecret(secret, position + 1);
        return;
    }
    secret[position] += 1;
}


int main() {
    unsigned long ID[batchSize];
    char secret[secretBufferSize * batchSize];
    struct timeval tstart, tend;
    long seconds, micros;
    double timeInterval;

    srand(time(NULL) * 80 + 34634);
    bool end = false;

    int i;

    // memcpy(secret,    "pWME8qhktupYnNxwhxLVr7rUB1UpXqkcaUG7aMhu6vgh6CEdZdYfadZz5JPSXXUWHpTG1Y0Vj42fwuUu", secretBufferSize);
    // memcpy(secret+secretBufferSize, "ly5py9cW4LBKxmvjTK0nzOdkY6a3qvaAT4Nf1P1VpqtMCOTkmJxLnzTbvU3Lf1aYVN1MrTvG7PgArZUu", secretBufferSize);
    // memcpy(secret+2*secretBufferSize,"Fy5dJuXJNHCnKtlwdWnPd49H2OLq5AYxWTBuco3NTvYrM9LPWYtXRsvJ46XXvLlfsKzsWs6FLSUmSvDu", secretBufferSize);

    for (i=0; i < batchSize; i++) {
        randstring(&secret[secretBufferSize*i], secretBufferSize);
    }

    // Remember to hardcode batchSize at programConstants.h
    sha256_init();

    printf("Searching...\n");

    unsigned long rounds = 0;
    gettimeofday(&tstart, NULL);
    do {
        for (i=0; i < batchSize; i++) {
            incSecret(secret+secretBufferSize*i, 0);
        }
        // for (i=0; i < batchSize; i++) {
        //     printf("%s\n", secret+secretBufferSize*i);
        // }
        sha256_crypt(secret, ID);
        ++rounds;
        if ( (rounds % 1000) == 0L) {
            gettimeofday(&tend, NULL);
            seconds = (tend.tv_sec - tstart.tv_sec);
            micros = ((seconds * 1000000) + tend.tv_usec) - (tstart.tv_usec);
            timeInterval = (double)micros/1000000;
            printf("\r %lu: %f tries/second in %f seconds", rounds*batchSize, (double)(1000*batchSize)/ timeInterval, timeInterval);
            fflush(stdout);
            gettimeofday(&tstart, NULL);
        }
        for (i=0; i< batchSize; i++) {
            if (ID[i] > 0xffff000000000000) {
                end = true;
                break;
            }
        }
    } while (end == false);
    printf("\nFound in %lu rounds\n", rounds * batchSize);
    printf("\nPassphrase: '%*.*s' id: %lu\n", secretBufferSize, secretBufferSize, secret + i*secretBufferSize, ID[i]);

    // for (i=0; i< batchSize; i++) {
    //     printf("'%*.*s': %lu\n", secretBufferSize, secretBufferSize, secret + i*secretBufferSize, ID[i]);
    // }

}
