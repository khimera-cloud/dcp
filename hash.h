//get xxHash for file, but only "until" bytes
XXH128_canonical_t* getFileHash(char* fname, unsigned long until, int verbose);

char* humanDigest(XXH128_canonical_t* hash);

//Blocksize to hash in chunks by a hashing thread
//The larger it is, the less Hash algos are ran, but the more data moved if one differs
//ALWAYS should be a multiply of BUFSIZE - the READ buffer, or BAAAD things can happen
//This is 20MB, 20 times BUFSIZE which is 1MB
#define HASHBLOCK BUFSIZE*20

//This is the structure for a hashItem in HashList
typedef struct hashItem {
 unsigned long offset;
 unsigned long length;
 XXH128_canonical_t digest;
} hashItem;


//This will be pointer structure which is dynamically allocated by the hashing thread
//by the size of the file, chunks, number of cores and HASHBLOCK
//Also all hashes list of a file can be stored in this sequentially
typedef struct hashList {
 unsigned long hashNo;
 hashItem* hash[];
} hashList;

//Get hashList (HASH Table) for fname until until bytes (block devices)
hashList* getFileHashChunks(char* fname, unsigned long until, int verbose, int onethread);

//This struct is passed to a hashing thread
typedef struct threadHashInfo {
 pthread_t id;
 unsigned long current; //the current number the thread is working on in its hashL
 char* fn;
 unsigned long start, end;
 hashList* hashL;
} threadHashInfo;

//Function to run for every thread / chunk
void* threadHash(void* arg);

//Data structure for info display thread
typedef struct threadDisInfo {
 pthread_t id;
 int cpus;
 threadHashInfo *others;
} threadDisInfo;

//display status of other threads
void* threadDisplay(void* arg);

//sha256 hash of file
char* sha256sum(char *fname, unsigned long until, int verbose);
