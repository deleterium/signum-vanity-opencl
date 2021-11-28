# Signum vanity opencl
Tentative to use opencl to speed up account ID creation.

# Status
Currently it works but only one passphrase at a time, not taking advantage of gpu cores processing. It is very slow at 66 tries per second.

# Compile
Git clone then `gcc main.cpp sha256.c -lOpenCL && ./a.out`

# Inspiration
* https://github.com/PlasmaPower/nano-vanity/ Project that uses also curve25519 for cryptography. Can reach 130k tries per second (on RX470)
* https://github.com/Fruneng/opencl_sha_al_im Skeleton for working sha256 function for this project
* https://github.com/ohager/signum-vanity-address-generator Uses java and can reach 11k tries per thread per second on ryzen 3.

# Notes
I tried to implement signum-vanity-address-generator in C to speed up, but got only 5% improvement. From this code that I derived the opencl code.
