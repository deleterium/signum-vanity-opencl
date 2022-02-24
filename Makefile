MY_CFLAGS = -O2 -Wextra

all: compile src/main.c
	gcc -o build/vanity src/main.c build/*.o $(MY_CFLAGS) -lm -lcrypto -lOpenCL && \
	rm -f dist/* && \
	cp build/vanity resources/* *.md dist/

compile: build/ed25519-donna.o build/cpu.o build/gpu.o \
		build/ReedSolomon.o build/argumentsParser.o

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

clean:
	rm -f build/* && rm -f dist/*
