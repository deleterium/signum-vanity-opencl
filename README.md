# Signum vanity opencl
Use GPU or CPU to find a Signum address that matches your desire.

# Status
Hashing at 2M tries per second on RX470. This is 33 times faster than using cpu, or 180x faster than usign previous java solutions (on my setup)! Optimized code in C works at 60k tries per second, 5.5 times faster than java vanity.

# Binary package usage

## Linux
* Extract package contents
* Test if it is running on cpu: `./vanity --cpu A?A`
* Play with command line options to maximize speed

## Windows
* Extract package contents
* Open a command prompt
* Go to the folder
* Test it running on cpu: `vanity --cpu A?A`
* Play with command line options to maximize speed

# Help
```
Passphrase generator for vanity addresses on Signum cryptocurrency.

Usage: vanity [OPTION]... MASK
Example: vanity --cpu --pass-length 32 SGN@

Options:
  --help             Show this help statement
  --suffix           Match given mask from the end of address. Default is to match from the beginning
  --pass-length N    Passphrase length. Max 134 chars. Default: 64
  --cpu              Set to use CPU and disable using GPU
  --gpu              Set to use GPU. Already is default
  --gpu-platform N   Select GPU from platorm N. Default: 0
  --gpu-device N     Select GPU device N. Default: 0
  --gpu-threads N    Send a batch of N threads. Default: 16384
  --gpu-work-size N  Select N concurrent works. Default: 64
  --endless          Never stop finding passphrases
  --use-charset ABC  Generate passphrase only containing the ABC chars
  --use-bip39        Generate passphrase with 12 words from BIP-39 list
  --dict [EN|PT|ES]  Dictionary language for bip-39. Default is english
  --add-salt ABC     Add your salt to the bip39 word list
  --append-db        Append (or create) database.csv with found results

Mask:
  Specify the rules for the desired address.
  It must be at least one char long.
  No 0, O, I or 1 are allowed.
  Following wildcars can be used:
    ?: Matches any char
    @: Matches only letters [A-Z]
    #: Matches only numbers [2-9]
    c: Matches consonants [BCDFGHJKLMNPQRSTVWXZ]
    v: Matches vowels [AEUY]
    p: Matches previous char
    -: Use to organize the mask, does not affect result
```

* Define a custom charset, custom length and your salt for paranoid levels of security
* Use gpu-work-size as multiple of you graphics card compute units, and gpu-threads to multiple of gpu-work-size for best speed.
* Try to rename the .cl file and swap with the second implementation. It can be 5% faster.
* More support on my discord channel: [SmartC Channel](https://discord.gg/pQHnBRYE5c).
* Drop some donation: S-DKVF-VE8K-KUXB-DELET.

# Compilation details

## Linux
* Dependencies: build-essential, OpenSSL and OpenCL driver installed for your graphics card
* Clone repository
* Compile `gcc -o vanity main.c cpu.c gpu.c ReedSolomon.c ed25519-donna/ed25519.c argumentsParser.c -m64 -lcrypto -lOpenCL -lm -O2 -Wextra`

## Windows
* Dependencies: (Visual Studio Community Edition)[https://visualstudio.microsoft.com/vs/community/], (OpenSSL library)[https://slproweb.com/products/Win32OpenSSL.html] and OpenCL SDK for your graphics card: (AMD)[https://github.com/GPUOpen-LibrariesAndSDKs/OCL-SDK] (Compilation not tested on NVIDIA or Intel graphics)
* Clone repository
* Open the solution
* Adjust headers/libraries path according to the installed dependencies folders `include` and `lib`.
* Compile solution

## Resource files
* Remember to include `passphraseToId.cl`, `passphraseToId2.cl` and `README.md` with the binary file if redistributing a compiled version.

# Inspiration
* https://github.com/PlasmaPower/nano-vanity/ Project that uses also curve25519 for cryptography. It also reaches 2M tries per second (on RX470).
* https://github.com/Fruneng/opencl_sha_al_im Skeleton for working sha256 function for this project.
* https://github.com/ohager/signum-vanity-address-generator Uses java and can reach 11k tries per thread per second on ryzen 3.
* https://github.com/floodyberry/ed25519-donna Highly optimized algorithm for curve25519.
* https://github.com/bkerler/opencl_brute Source of full working C algorithm for sha-256.
