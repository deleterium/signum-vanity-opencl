# Signum vanity opencl
Use opencl or optimized code for CPU to speed up account ID creation.

# Status
Hashing at 1M tries per second on RX470! Cpu algorithm hashes at 15k tries per second, 36% faster than java vanity with ryzen 3 1300.

# Compile && run
Git clone then `gcc -o vanity main.c gpu.c ed25519-donna/ed25519.c -m64 -lcrypto -lOpenCL -lm && ./vanity`

# Help
```
Usage: vanity [OPTION [ARG]] ... MASK
  --help             show this help statement
  --pass-length N    Passphrase length. 40 to 120 chars. Default: 64
  --cpu              Set to use CPU. Using it disables using GPU.
  --gpu              Set to use GPU. Default is to use.
  --gpu-platform N   Select GPU from platorm N
  --gpu-device N     Select GPU device N
  --gpu-threads N    Send a batch of N threads
  --gpu-work-size N  Select N concurrent works
  --endless          Never stop finding passphrases

  MASK   Desired address. Use '_' for any char at that location. Must be at least one char and maximum 17 chars. No 0, O, I or 1 allowed.

Example: vanity --gpu --gpu-threads 102400 --gpu-work-size 32 V_A_N_I
```

# Inspiration
* https://github.com/PlasmaPower/nano-vanity/ Project that uses also curve25519 for cryptography. Can reach 130k tries per second (on RX470).
* https://github.com/Fruneng/opencl_sha_al_im Skeleton for working sha256 function for this project (actually it is not working on big passphrases).
* https://github.com/ohager/signum-vanity-address-generator Uses java and can reach 11k tries per thread per second on ryzen 3.
* https://github.com/floodyberry/ed25519-donna Highly optimized algorithm for curve25519.
* https://github.com/bkerler/opencl_brute Source of full working C algorithm for sha-256.

