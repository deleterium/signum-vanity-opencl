#include "sha256.h"
#include "time.h"

void crypt_and_print(char* input) 
{
  unsigned long ID;
  ID = sha256_crypt(input);
  printf("'%s': %lu\n", input, ID);
}

void randstring(char * buffer, size_t length) {

    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";        

    if (length) {
        for (int n = 0;n < length;n++) {            
            int key = rand() % (int)(sizeof(charset) -1);
            buffer[n] = charset[key];
        }
        buffer[length] = '\0';
    }
}

void incSecret(char * secret, size_t position) {
    
    if (secret[position] >= 'z') {
        secret[position] = 'a';
        incSecret(secret, position + 1);
        return;
    }
    secret[position] += 1;
}

int main() {
    unsigned long ID;
    char secret[80];
    printf("Searching.");
    srand(time(NULL) * 80 + 34634);
    randstring(secret, 79);

    sha256_init(2048);

    unsigned long rounds = 0;
    do {
        incSecret(secret, 0);
        
        ID = sha256_crypt(secret);

        ++rounds;
        if ( (rounds % 1000) == 0L) {
            printf(".");
            fflush(stdout);
        }
    } while (ID < 0xfff0000000000000);
    printf("\nFound in %lu rounds\n", rounds);
    printf("\n'%s': %lu\n", secret, ID);
}
