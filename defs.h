#define VERSION "0.2.1"
#define URL "https://github.com/khimera.cloud/dcp"
#define LICENSES XXHASH_LIC "\n" OPENSSL_LIC "\n" GLIBC_LIC "\n"
#define XXHASH_LIC "xxHash by Yann Collet - https://cyan4973.github.io/xxHash/"
#define OPENSSL_LIC "OpenSSL Project - https://openssl.org/"
#define GLIBC_LIC "GNU C Library - https://gnu.org/software/libc/"

//The BUFSIZE will be allocated per CPU Core for every thread when comparing parts of a file
//#define BUFSIZE 1048576

extern long BUFSIZE;

extern long HASHBLOCK;

#define ex_err(...) { printf(__VA_ARGS__); exit(EXIT_FAILURE); }
