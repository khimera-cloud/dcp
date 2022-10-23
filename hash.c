#define _GNU_SOURCE //cpu_set_affinity_np() for pthread.h
#include <stdio.h> //sprintf(), fread(), etc
#include <stdlib.h> //malloc()
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <xxhash.h> //xxHash
#include <sys/sysinfo.h> //get_nprocs()
#include <pthread.h> //pthread_create(), etc
#include <openssl/sha.h> //sha256

#include "defs.h"
#include "hash.h"

//////////////////////////////////////////////////////////////////////////////////////
// Get hash of one file

XXH128_canonical_t* getFileHash(char* fname, unsigned long until, int verbose) {
 //Init hash state
 XXH3_state_t* state = XXH3_createState();
 XXH3_128bits_reset(state);
 FILE* file;
 unsigned char* buffer = malloc(BUFSIZE);
 int bytesRead, perc;
 file = fopen(fname, "rb");
 unsigned long allBytes = 0;

 unsigned long cycles = until/BUFSIZE;
 unsigned long rema = until%BUFSIZE;

 for (unsigned long i=0; i<cycles; i++) {
  bytesRead = fread(buffer, 1, BUFSIZE, file);
  //Update the hash with data read
  XXH3_128bits_update(state, buffer, bytesRead);
  allBytes+=bytesRead;
  if (verbose) {
   if ((until/100)==0) perc = 0; //to avoid division by 0 in next statement
   else perc = allBytes/(until/100);
   if (perc>100) perc = 100; //because it CAN happen :)
   printf("\33[2K\rCalculating xxHash %i%%", perc);
   fflush(stdout);
  }
 }
 if (verbose) printf("\33[2K\r");
 if (rema>0) {
  bytesRead = fread(buffer, 1, rema, file);
  XXH3_128bits_update(state, buffer, bytesRead);
 }

 fclose(file);
 free(buffer);

 //Finalize
 XXH128_hash_t hash = XXH3_128bits_digest(state);

 //Free memory
 XXH3_freeState(state);

 //Should be Big Endian by default (x86-64 Linux)
// printf("%s: %016lx%016lx\n", fname, hash.high64, hash.low64);

 //Convert hash to "canonical" (standardized) form, always Big Endian on all platforms
 XXH128_canonical_t* ret_hash = malloc(sizeof(XXH128_canonical_t));
 XXH128_canonicalFromHash(ret_hash, hash);

 return(ret_hash);
}

//////////////////////////////////////////////////////////////////////////////////////
// Create human readable (16 hex bytes) digest

char* humanDigest(XXH128_canonical_t* hash) {
 //Display (canonical) XXH128 digest as hex bytes
 char* output = malloc(33);
 for (int i=0; i<16; i++) sprintf(output+(i*2), "%02x", hash->digest[i]);
 output[32]=0;
 return(output);
}

//////////////////////////////////////////////////////////////////////////////////////
// Get a chunked list of hashes of a file

hashList* getFileHashChunks(char* fname, unsigned long until, int verbose, int onethread) {
 int cpus = 1;
 if (onethread == 0) cpus = get_nprocs();

#ifdef DEBUG
 printf("CPUs found: %i\n", cpus);
#endif

 //1 THREAD DEBUG TEST
// cpus=1;

 //Cut file (or until we are working) by cpu number of times to chunks
 unsigned long chunk=until/cpus;

 //If chunk size is less than a buffered read per core, use only one thread - for small files
 if (chunk<(BUFSIZE*cpus)) cpus=1;

#ifdef DEBUG
 printf("until: %li / cpus (chunk): %lu <> Read: %i -> CPUs to use: %i\n", until, chunk, BUFSIZE, cpus);
#endif

 if (verbose) printf("Starting #%i threads to calculate xxHash table for %s\n", cpus, fname);
 //Start cpu number of threads to calculate every chunk
 threadHashInfo runThreads[cpus];
 for (int i=0; i<cpus; i++) {
  runThreads[i].fn = fname;
  runThreads[i].current = 0;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(i, &cpuset); //put core #i (i.e. 0->max core-1) to a cpu_set

  //First thread always starts at zero
  if (i==0) runThreads[i].start = 0;
  else runThreads[i].start = runThreads[i-1].end+1;
  //Last thread always ends at unsigned long "until", even if chunk is smaller / larger
  if (i==cpus-1) runThreads[i].end=until-1;
  else runThreads[i].end=runThreads[i].start+chunk-1;
  pthread_create(&runThreads[i].id, NULL, &threadHash, &runThreads[i]);

  int s=pthread_setaffinity_np(runThreads[i].id, sizeof(cpu_set_t), &cpuset); //set affinity for thread
  if (s!=0) ex_err("Set CPU Affinity failed\n");

 }

 threadDisInfo tdInfo;
 tdInfo.cpus = cpus;
 tdInfo.others = runThreads;

 if (verbose) pthread_create(&tdInfo.id, NULL, &threadDisplay, &tdInfo);

 hashList* fileHashes;
 unsigned long allHashNo = 0, curHashNo = 0;

 for (int i=0; i<cpus; i++) {
  pthread_join(runThreads[i].id, NULL);
  allHashNo+=runThreads[i].hashL->hashNo;
#ifdef DEBUG
  printf("Thread %lu join. HashNo is %lu\n", runThreads[i].id, runThreads[i].hashL->hashNo);
#endif
 }
 if (verbose) pthread_join(tdInfo.id, NULL);

 fileHashes = malloc(sizeof(unsigned long) + allHashNo*sizeof(hashItem*)); //pointer
 for (unsigned long i=0; i<allHashNo; i++) fileHashes->hash[i] = malloc(sizeof(hashItem)); //actual value
 fileHashes->hashNo = allHashNo;

//copy values from every thread's return memory to this functions return
 for (int i=0; i<cpus; i++) {
  for (unsigned long j=0; j<runThreads[i].hashL->hashNo; j++) {
   memcpy(&fileHashes->hash[curHashNo]->digest, &runThreads[i].hashL->hash[j]->digest, sizeof(XXH128_canonical_t));
   fileHashes->hash[curHashNo]->offset = runThreads[i].hashL->hash[j]->offset;
   fileHashes->hash[curHashNo]->length = runThreads[i].hashL->hash[j]->length;
#ifdef DEBUG
   char* hd = humanDigest(&runThreads[i].hashL->hash[j]->digest);
   printf("Thread %i Hash %lu is %s, copied to %lu\n", i, j, hd, curHashNo);
   free(hd);
#endif
   curHashNo++;
   free(runThreads[i].hashL->hash[j]);
  }
  free(runThreads[i].hashL);
 }
 return fileHashes;
}

//////////////////////////////////////////////////////////////////////////////////////
// Thread function

void* threadHash(void* arg) {
 threadHashInfo* thread = arg;

 //Init hash state
 XXH3_state_t* state = XXH3_createState();
 XXH3_128bits_reset(state);

 FILE* file;
 unsigned char* buffer = malloc(BUFSIZE);
 unsigned long chunk = thread->end-thread->start+1;
 int bytesRead;
 unsigned long HB = HASHBLOCK;
#ifdef DEBUG
 printf("%li: my chunk: %lu - HB: %lu\n", thread->id, chunk, HB);
#endif

 //calculate number of hashes to be created from the size of our chunk and the HASHBLOCK size
 unsigned long allHashes=chunk/HB, curHash = 0, hashBlockRead = 0;
 if ((chunk%HB)>0) allHashes++;
#ifdef DEBUG
 unsigned long allBytes = 0;
#endif

 //Malloc for the pointer structure to be reutrned from this thread
 thread->hashL = malloc(sizeof(unsigned long) + allHashes*sizeof(hashItem*)); //pointer
 for (int i=0; i<allHashes; i++) thread->hashL->hash[i] = malloc(sizeof(hashItem));
 thread->hashL->hashNo = allHashes;

 //Number of read cycles to be done with buffered read,
 //and a last remainer if smaller then bufsize but greater than zero
 unsigned long cycles = chunk/BUFSIZE;
 unsigned long rema = chunk%BUFSIZE;
#ifdef DEBUG
 printf("%li: %lu read cycles, and %lu remainder as +1, # of hashes: %lu\n", thread->id, cycles, rema, allHashes);
#endif

 file = fopen(thread->fn, "rb");
 fseek(file, thread->start, SEEK_SET);
 //set (starting) position - curHash is 0 now, thread->start where reading starts
 thread->hashL->hash[curHash]->offset = thread->start;

#ifdef DEBUG
 printf("%li (# %lu) starting pos: %lu\n", thread->id, curHash, thread->hashL->hash[curHash]->offset);
#endif

 for (unsigned long i=0; i<cycles; i++) {
  bytesRead = fread(buffer, 1, BUFSIZE, file);
#ifdef DEBUG
  allBytes+=bytesRead;
#endif
  XXH3_128bits_update(state, buffer, bytesRead);
  //THIS SHOULD NOT HAPPEN, as HASH BLOCK SIZE needs to be a multiply of read buffer size BUFSIZE
  if ((hashBlockRead+bytesRead) > HB) ex_err("Read more bytes than HASHBLOCK, the impossible happened?\n");
  if ((hashBlockRead+bytesRead) < HB) hashBlockRead+=bytesRead;
  else { // This can only happen at equals, as we exited before if it was more
   XXH128_canonicalFromHash(&thread->hashL->hash[curHash]->digest, XXH3_128bits_digest(state));
   thread->hashL->hash[curHash]->length = hashBlockRead+bytesRead;
#ifdef DEBUG
   char* hd = humanDigest(&thread->hashL->hash[curHash]->digest);
   printf("%li (# %lu) %s (L: %lu) - read %lu\n", thread->id, curHash, hd, thread->hashL->hash[curHash]->length, allBytes);
   free(hd);
#endif
   XXH3_128bits_reset(state);
   hashBlockRead=0;
   thread->current++; //this holds the same value as curHash below, but that is easier to manage
   curHash++;
   //set NEXT hash starting position, if there is one
   if (i+1<cycles) {
    thread->hashL->hash[curHash]->offset = ftell(file);
#ifdef DEBUG
    printf("%li (# %lu) starting pos: %lu\n", thread->id, curHash, thread->hashL->hash[curHash]->offset);
#endif
   }
  }
 }

 //rema=0 means chunk size aligns buffer read size, it rema > 0, we have 1 more fread to do
 //curHash<allHashes means we have read everything needed,
 //but the last hash is smaller than the standard hashblock, so it wasn't finalized
 if ((rema > 0) || (curHash<allHashes)) {
  //we read rema bytes, can be 0, it doesnt alter anything
  bytesRead = fread(buffer, 1, rema, file);
#ifdef DEBUG
  printf("Remaining / Half buffered condition found\n");
  allBytes+=bytesRead;
#endif
  //Update with the above read bytes, this again can be zero,
  //if rema was 0 but the last hash was smaller than HASHBLOCK
  XXH3_128bits_update(state, buffer, rema);
  //Do the last finalized canon hash, from the last state
  XXH128_canonicalFromHash(&thread->hashL->hash[curHash]->digest, XXH3_128bits_digest(state));
  thread->hashL->hash[curHash]->length = hashBlockRead+rema; //same as +bytesRead above
  thread->current++; //So the display thread can exit :)
#ifdef DEBUG
  char* hd = humanDigest(&thread->hashL->hash[curHash]->digest);
  printf("%li (# %lu) %s (L: %lu) - read %lu\n", thread->id, curHash, hd, thread->hashL->hash[curHash]->length, allBytes);
  free(hd);
#endif
 }

 fclose(file);
 free(buffer);
 XXH3_freeState(state);
#ifdef DEBUG
 printf("%li bytes read all by thread: %lu - last hash index: %lu\n", thread->id, allBytes, curHash);
#endif
 return thread;
}

//////////////////////////////////////////////////////////////////////////////////////
// Thread function for displaying

void* threadDisplay(void* arg) {
 threadDisInfo* thread = arg;

#ifdef DEBUG
 printf("Display thread: CPUS: %i\n", thread->cpus);
#endif

 //wait until the first hash is calculated for every thread,
 //because hashL->hashNo may not have been malloc'd until then
 for (int i=0; i<thread->cpus; i++) {
  int first_done=0;
  do {
   if (thread->others[i].current>0) first_done++;
   usleep(20000); //20,000 microseconds, 0.02 second
  } while(first_done==0);
 }

 int done = 0;
 while (done<thread->cpus) {
  for (int i=0; i<thread->cpus; i++) {
   printf("\33[2KThread #%i: %lu/%lu\n", i, thread->others[i].current, thread->others[i].hashL->hashNo);
   if (thread->others[i].current==thread->others[i].hashL->hashNo) done++;
  }
  for (int i=0; i<thread->cpus; i++) {
   printf("\033[A");
  }
  usleep(100000); //100,000 microseconds, 0.1 second
 }
 for (int i=0; i<thread->cpus; i++) {
  printf("\033[2K\n");
 }
 for (int i=0; i<thread->cpus; i++) {
  printf("\033[A");
 }
 fflush(stdout);
 return thread;
}

char* sha256sum(char *fname, unsigned long until, int verbose) {
 unsigned char hash[SHA256_DIGEST_LENGTH];
 unsigned char* buffer = malloc(BUFSIZE);
 unsigned long allBytes = 0;
 int bytesRead, perc;
 SHA256_CTX sha256;
 SHA256_Init(&sha256);
 FILE* file = fopen(fname, "rb");
 if (file == NULL) { //doesn't exist, whatever
  free(buffer);
  char *ret = malloc(1);
  ret[0] = 0;
  return ret;
 }

 unsigned long cycles = until/BUFSIZE;
 unsigned long rema = until%BUFSIZE;

 for (unsigned long i=0; i<cycles; i++) {
  bytesRead = fread(buffer, 1, BUFSIZE, file);
  SHA256_Update(&sha256, buffer, bytesRead);
  allBytes+=bytesRead;
  if (verbose) {
   if ((until/100)==0) perc = 0; //to avoid division by 0 in next statement
   else perc = allBytes/(until/100);
   if (perc>100) perc = 100; //because it CAN happen :)
   printf("\33[2K\rCalculating SHA256 %i%%", perc);
   fflush(stdout);
  }
 }
 if (verbose) printf("\33[2K\r");
 if (rema>0) {
  bytesRead = fread(buffer, 1, rema, file);
  SHA256_Update(&sha256, buffer, bytesRead);
 }
 SHA256_Final(hash, &sha256);

 free(buffer);
 fclose(file);

 char* output = malloc(SHA256_DIGEST_LENGTH*2+1); // +1 for \0 terminator character (call it Arnie)
 for (int i=0; i<SHA256_DIGEST_LENGTH; i++) sprintf(output+(i*2), "%02x", hash[i]);
 output[SHA256_DIGEST_LENGTH*2]=0;

 return output;
}
