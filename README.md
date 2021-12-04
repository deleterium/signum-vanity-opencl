# Signum vanity opencl
Use opencl or optimized code for CPU to speed up account ID creation.

# Status
Hashing at 30k tries per second on RX470! Cpu algorithm hashes at 15k tries per second, 36% faster than java vanity with ryzen 3 1300.

# Compile && run
Git clone then `gcc main.cpp gpu.c ed25519-donna/ed25519.c -m64 -lcrypto -lOpenCL && ./a.out`

# Inspiration
* https://github.com/PlasmaPower/nano-vanity/ Project that uses also curve25519 for cryptography. Can reach 130k tries per second (on RX470).
* https://github.com/Fruneng/opencl_sha_al_im Skeleton for working sha256 function for this project (actually it is not working on big passphrases).
* https://github.com/ohager/signum-vanity-address-generator Uses java and can reach 11k tries per thread per second on ryzen 3.
* https://github.com/floodyberry/ed25519-donna Highly optimized algorithm for curve25519.
* https://github.com/bkerler/opencl_brute Source of full working C algorithm for sha-256.

# Notes
Still no command line arguments and no RS address. Hashing only to IDs.
