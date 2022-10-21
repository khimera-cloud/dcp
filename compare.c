#include <stdio.h> //printf()
#include <stdlib.h> //malloc(), free()
#include <string.h> //memcmp()
#include <unistd.h> //truncate()
#include <time.h> //clock()
#include <math.h> //round()
#include <xxhash.h> //XXHash

#include "defs.h"
#include "file.h"
#include "compare.h"
#include "hash.h"

void compare(workFile srct, workFile dstt, int verbose, int dry, int sha, int copy, int skipxx) {

 //SOURCE info

 if (verbose) {
  if (srct.type==-1) printf("Source file can't be opened, ");
  if (srct.type==0) printf("Source file type unknown, ");
  if (srct.type==1) printf("Source file type is block, ");
  if (srct.type==2) printf("Source file type is regular, ");
  char* hs = humanSize(srct.size);
  printf("size is %lu bytes (%s)\n", srct.size, hs);
  free(hs);
 }
 if (srct.type<=0) ex_err("Source file is neither block or regular!\n");
 if (srct.size==0) ex_err("Source file is empty!\n");

 //DESTINATION INFO

 if (verbose) {
  if (dstt.type==-1) printf("Desti. file can't be opened, ");
  if (dstt.type==0) printf("Desti. file type unknown, ");
  if (dstt.type==1) printf("Desti. file type is block, ");
  if (dstt.type==2) printf("Desti. file type is regular, ");
  char* hs = humanSize(dstt.size);
  printf("size is %lu bytes (%s)\n", dstt.size, hs);
  free(hs);
 }
 if (dstt.type==0) ex_err("Destination file is neither block or regular!\n");

//Make the 2 files the same size before comparing, OR compare unly up until source size

 if (srct.size!=dstt.size) {
  if ((verbose) || (dry)) printf("File sizes differ.\n");
  if (dry) {
   if (sha) calcBothSha(srct.name, dstt.name, srct.size, verbose);
   printf("Dry-run, not doing anything more...\n");
   exit(EXIT_SUCCESS);
  }
  if ((dstt.type==-1) || (dstt.size==0)){ //error
   printf("Destination is empty or doesn't exist, starting clone function...\n");
   char* hs = humanSize(copyBytes(srct.name, dstt.name, 0, srct.size, verbose));
   printf("Cloned %s.\n", hs);
   free(hs);
   if (sha) calcBothSha(srct.name, dstt.name, srct.size, verbose);
   exit(EXIT_SUCCESS);
  }
  if (dstt.type==1) { //block device
   printf("Destination type is block, it's size can't be changed to the source's!\n");
   if (srct.size>dstt.size) {
    printf("Destination device is smaller, can't copy!\n");
    exit(EXIT_FAILURE);
   } else {
    if (verbose) printf("Destination device is larger (only comparing up until source file's size)!\n");
   }
  }
  if (dstt.type==2) { //regular file
   if (srct.size>dstt.size) {
    printf("Destination file is smaller, copying the end of source...\n");
    char* hs = humanSize(copyBytes(srct.name, dstt.name, dstt.size, srct.size-dstt.size, verbose));
    printf("Copied %s.\n", hs);
    free(hs);
   } else {
    printf("Destination file is larger, truncating it's end...\n");
    if (truncate(dstt.name, srct.size)) ex_err("Truncate error!\n");
   }
  }
 }

 //COMPARE WHOLE

 if (verbose) printf("Comparing (and differential cloning) %s->%s\n", srct.name, dstt.name);

 clock_t start;
 if (skipxx==0) {

  start = clock();
  XXH128_canonical_t* srcH = getFileHash(srct.name, srct.size, verbose);

  if (verbose) {
   char* hd = humanDigest(srcH);
   printf("Source xxHash: %s, ", hd);
   free(hd);

   clock_t end = clock();
   unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
   printf("calculation time: %lu seconds.\n", elapsed);
  }

  start = clock();
  XXH128_canonical_t* dstH = getFileHash(dstt.name, srct.size, verbose);

  if (verbose) {
   char* hd = humanDigest(dstH);
   printf("Desti. xxHash: %s, ", hd);
   free(hd);

   clock_t end = clock();
   unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
   printf("calculation time: %lu seconds.\n", elapsed);
  }

  if (!(memcmp(srcH->digest, dstH->digest, sizeof(XXH128_canonical_t)))) {
   free(srcH);
   free(dstH);
   printf("The 2 files are the same!\n");
   if (sha) calcBothSha(srct.name, dstt.name, srct.size, verbose);
   exit(EXIT_SUCCESS);
  }

  free(srcH);
  free(dstH);

  //The 2 files differ
  if ((verbose) || (dry)) printf("The 2 files differ!\n");
 } else {
  if (verbose) printf("Skipping full file xxHash, calculating xxHash tables.\n");
 }

 if (dry) {
  if (sha) calcBothSha(srct.name, dstt.name, srct.size, verbose);
  printf("Dry-run, not doing anything more...\n");
  exit(EXIT_SUCCESS);
 }

 //The 2 files differ, copy only up to until source size (if dst is block AND larger)
 if (copy) {
  if (verbose) printf("Copy parameter detected, doing full copy.\n");
  char* hs = humanSize(copyBytes(srct.name, dstt.name, 0, srct.size, verbose));
  printf("Cloned %s.\n", hs);
  free(hs);
  if (sha) calcBothSha(srct.name, dstt.name, srct.size, verbose);
  exit(EXIT_SUCCESS);
 }

 //CALC source HASH table
 start = clock();
 hashList* srcHL = getFileHashChunks(srct.name, srct.size, verbose);

 if (verbose) {
  clock_t end = clock();
  unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
  printf("Source xxHash table calculation time: %lu seconds, items: %lu\n", elapsed, srcHL->hashNo);
 }

 //CALC dest HASH table
 start = clock();
 hashList* dstHL = getFileHashChunks(dstt.name, srct.size, verbose);

 if (verbose) {
  clock_t end = clock();
  unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
  printf("Desti. xxHash table calculation time: %lu seconds, items: %lu\n", elapsed, dstHL->hashNo);
 }

 //If the files don't change size, and the number of CPUs are not changing,
 //than this shouldn't happen!!!
 if (srcHL->hashNo!=dstHL->hashNo) ex_err("The 2 calcluated xxHash Tables differ in number items???\n");

 unsigned long allBytes=0, diffs=0;
 start = clock();
 for (unsigned long i=0; i<srcHL->hashNo; i++) {
  if ((memcmp(&srcHL->hash[i]->digest, &dstHL->hash[i]->digest, sizeof(XXH128_canonical_t)))) {
#ifdef DEBUG
   char* hd1 = humanDigest(&srcHL->hash[i]->digest);
   char* hd2 = humanDigest(&dstHL->hash[i]->digest);
   printf("xxHashTable item %lu difference (%s<>%s)\n", i, hd1, hd2);
   free(hd1);
   free(hd2);
#endif
   //These 2 also shouldn't happen
   if (srcHL->hash[i]->offset!=dstHL->hash[i]->offset) ex_err("Offset mismatch!\n");
   if (srcHL->hash[i]->length!=dstHL->hash[i]->length) ex_err("Length mismatch!\n");
   allBytes+=copyBytes(srct.name, dstt.name, srcHL->hash[i]->offset, srcHL->hash[i]->length, verbose);
   diffs++;
  }
 }
 char* hs = humanSize(allBytes);
 clock_t end = clock();
 unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
 printf("Copied a total of %s in %lu blocks in %lu seconds.\n", hs, diffs, elapsed);
 free(hs);

 for (unsigned long i=0; i<srcHL->hashNo; i++) {
  free(srcHL->hash[i]);
  free(dstHL->hash[i]);
 }
 free(srcHL);
 free(dstHL);

 if (sha) calcBothSha(srct.name, dstt.name, srct.size, verbose);
}

//Copy n bytes from src to dst at offset
unsigned long copyBytes(char* src, char* dst, \
			unsigned long offset, unsigned long length, \
			int verbose) {

 FILE* srcf = fopen(src, "rb");
 if (srcf==NULL) ex_err("Can't open source for reading!\n");
 if (fseek(srcf, offset, SEEK_SET)<0) { perror("copyBytes src fseek"); exit(EXIT_FAILURE); }

 workFile srct = getFileInfo(src);

 FILE* dstf;
 //if copying the complete file, then opening it with creante and/or truncate mode
 if ((offset==0) && (length==srct.size)) dstf = fopen(dst, "wb");
 else dstf = fopen(dst, "r+b");
 if (dstf==NULL) ex_err("Can't open destination for writing!\n");
 if (fseek(dstf, offset, SEEK_SET)<0) { perror("copyBytes dst fseek"); exit(EXIT_FAILURE); }

 unsigned char* buffer = malloc(BUFSIZE);
 size_t didread, didwrite, bytesWritten = 0;
 int perc;
 clock_t start = clock();

 do {
  didread=fread(buffer, 1, BUFSIZE, srcf);
  //Write buffer, or remaining bytes (can be less, than buffer)
  if (bytesWritten+didread<=length) didwrite=fwrite(buffer, 1, didread, dstf);
  else didwrite=fwrite(buffer, 1, length-bytesWritten, dstf);

  if (didread>didwrite) { //write error
   if (verbose) printf("\n");
   ex_err("Write error!\n");
  }
  bytesWritten+=didwrite;

  if (verbose) {
   if ((length/100)==0) perc = 0; //to avoid division by 0 in next statement
   else perc = bytesWritten/(length/100);
   if (perc>100) perc = 100; //because it CAN happen :)
   char* hs = humanSize(bytesWritten);
   printf("\33[2K\rCopying: %lu bytes (%s), %i%%", bytesWritten, hs, perc);
   free(hs);
   fflush(stdout);
  }
 } while (bytesWritten<length);

 free(buffer);
 if (verbose) {
  clock_t end = clock();
  unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);

  printf("\33[2K\rCopied %lu bytes in %lu seconds, ", bytesWritten, elapsed);
  if (elapsed==0) elapsed++;
  char* hs = humanSize(bytesWritten/elapsed);
  printf("that's %s/s!\n", hs);
  free(hs);
 }

 fclose(srcf);
 fclose(dstf);

 return (bytesWritten);
}

//hash both files with SHA256, only until "until", because of block devices
void calcBothSha(char* src, char* dst, unsigned long until, int verbose) {
 clock_t start = clock();
 char* srcsha = sha256sum(src, until, verbose);
 printf("Source SHA256 hash: %s\n", srcsha);
 if (verbose) {
  clock_t end = clock();
  unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
  printf("Calculation time: %lu seconds.\n", elapsed);
 }
 free(srcsha);

 start = clock();
 char* dstsha = sha256sum(dst, until, verbose);
 printf("Desti. SHA256 hash: %s\n", dstsha);
 if (verbose) {
  clock_t end = clock();
  unsigned long elapsed = round((end - start)/CLOCKS_PER_SEC);
  printf("Calculation time: %lu seconds.\n", elapsed);
 }
 free(dstsha);
}
