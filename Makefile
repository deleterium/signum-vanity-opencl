MY_CFLAGS = -O2 -Wextra
MONGOC_CFLAGS = -I/usr/include/libmongoc-1.0 -I/usr/include/libbson-1.0
MONGOC_LIBS = -lmongoc-1.0 -lbson-1.0

all: compile src/main.c
	gcc -o build/vanity src/main.c build/*.o $(MY_CFLAGS) -lm -lcrypto -lOpenCL $(MONGOC_LIBS) && \
	rm -f dist/* && \
	cp build/vanity resources/*.cl *.md dist/

compile: build/ed25519-donna.o build/cpu.o build/gpu.o \
		build/ReedSolomon.o build/argumentsParser.o build/dbHandler.o

build/ed25519-donna.o: src/ed25519-donna/ed25519.c
	gcc -c -o build/ed25519-donna.o src/ed25519-donna/ed25519.c $(MY_CFLAGS)

build/cpu.o: src/cpu.c
	gcc -c -o build/cpu.o src/cpu.c $(MY_CFLAGS)

build/gpu.o: src/gpu.c
	gcc -c -o build/gpu.o src/gpu.c $(MY_CFLAGS)

build/ReedSolomon.o: src/ReedSolomon.c
	gcc -c -o build/ReedSolomon.o src/ReedSolomon.c $(MY_CFLAGS)

build/argumentsParser.o: src/argumentsParser.c
	gcc -c -o build/argumentsParser.o src/argumentsParser.c $(MY_CFLAGS)

build/dbHandler.o: src/dbHandler.c
	gcc -c -o build/dbHandler.o src/dbHandler.c $(MY_CFLAGS) $(MONGOC_CFLAGS)

clean:
	rm -f build/* && rm -f dist/*
