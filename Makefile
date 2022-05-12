MY_CFLAGS = -O2 -Wextra

# Detect Mac Os operating system and change option for opencl
ifeq ($(OS),Windows_NT)
    OPENCL_LIB = -lOpenCL
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        OPENCL_LIB = -framework OpenCL
	else
	  	OPENCL_LIB = -lOpenCL
    endif
endif

.PHONY: all clean pre-build

all: pre-build main-build

pre-build:
	mkdir -p {build,dist}

main-build: compile src/main.c
	gcc -o build/vanity src/main.c build/*.o $(MY_CFLAGS) -lm -lcrypto $(OPENCL_LIB) && \
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
