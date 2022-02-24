# Signum collision finder
Use GPU or CPU to find to passphrases that lead to same account ID.

# Status
Save random IDs (using a mask) to a mongodb collection.
When some error occurs to insert items, it could be that a collision was found. Check the files and query the db to get passphrases.

# Help
```
Signum ID collision finder

Usage: vanity [OPTION]... MASK
Example: vanity --cpu --pass-length 32 SGN@

Options:
  --help             Show this help statement
  --suffix           Match given mask from the end of address. Default is to match from the beginning
  --pass-length N    Passphrase length. Max 142 chars. Default: 64
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
  --append-db        Append (or create) a mongodb collection with found results
  --search-db        Search a mongodb collection with found results

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

# Compilation details

## Linux
* Dependencies: build-essential, OpenSSL, mongo-c-driver and OpenCL driver installed for your graphics card. Install and run a mongodb server.
* Clone repository
* Compile `make`
* The distribution files will be at 'dist' folder

## Windows
* NOT YET PORTED TO WINDOWS. Below instructions are not enough.
* Dependencies: (Visual Studio Community Edition)[https://visualstudio.microsoft.com/vs/community/], (OpenSSL library)[https://slproweb.com/products/Win32OpenSSL.html] and OpenCL SDK for your graphics card: (AMD)[https://github.com/GPUOpen-LibrariesAndSDKs/OCL-SDK] (Compilation not tested on NVIDIA or Intel graphics)
* Clone repository
* Open the solution
* Adjust headers/libraries path according to the installed dependencies folders `include` and `lib`.
* Compile solution

# Limits to work only on RAM
The ID is a 64-bit number, so it is expected collisions when a set of 2^32 records are avaliable for comparision.
The problem is that this 4 billion record will need around 160GB of RAM just for the index.
So use the mask to reduce the dataset to the maximum that you have.
For example, if you mask only address ending with 'Z', you will store on your database in average only 1 record each 32 are found: your dataset will have only (2^64) / 32 = 2^59.
It is expected to find a collision in 2^(n/2) documents, this means you will need to insert 2^(29.5) documents to have a high chance to find a collision: 2^(29.5) = 760 milion records.
These records will need 30.4 GB ram to be stored.
This is a good number if you have 32GB of RAM.
Because this dataset is reduce, you will need to increase the number of calculations: 760 million records times 32 = 24.3 billion ids!

NOTE: If your memory is already full, stop adding records and start only search with `--search-db`

NOTE: if you are using the free version of mongodb, remember that it is limited to half of you RAM minus 1 GB. So adjust the mask to this additional restriction.
