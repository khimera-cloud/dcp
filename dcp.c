#include <stdio.h> //printf()
#include <stdlib.h> //exit()
#include <string.h> //strcmp()

#include "defs.h"
#include "dcp.h"
#include "file.h"
#include "compare.h"

//1 MB per default for I/O
long BUFSIZE = 1048576;
//20 MB per default for Hash chunks, initialized as 0
long HASHBLOCK = 0;

void version(int license) {
 printf("dcp version %s, GPLv3\n", VERSION);
 printf("%s\n", URL);
 if (license) {
  printf("\nAdditional licenses:\n");
  printf(LICENSES);
 }
 exit(EXIT_SUCCESS);
}

void usage(char* errmsg) {
 if (errmsg) printf("%s\n\n", errmsg);
 printf("Usage: dcp [OPTION]... <src dev/file> <dest dev/file>\n");
 printf("Differential copy tool using multithreaded xxHash\n");

 printf("\nOptions:\n");
 printf("-h, --help		show this message and exit\n");
 printf("-l, --licenses		display licensing information and exit\n");
 printf("-V, --version		show version information and exit\n");
 printf("-v, --verbose		verbose mode\n");
 printf("-n, --dry		dry-run, no-opt mode\n");
 printf("-s, --sha256		calculate (and display) SHA256 hash after copy\n");
 printf("-c, --copy		always copy the whole while if differs\n");
 printf("-x, --skipxx		skip the xxHash on the whole file, just do xxHash tables\n");

 printf("\nAdvenced options:\n\n");

 printf("-1, --one-thread	use only one thread while computing the chunks' hashes\n");
 printf("    --io-buffer <N>	I/O buffer size for file operations in MB - Integer 1<=N<=100, default is 1\n");
 printf("    --hash-chunk <N>	Hash chunk size, multiplier of io-buffer - Integer 1<=N<=100, default is 20\n");

 if (errmsg) exit(EXIT_FAILURE);
 exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
 int verbose = 0, dry = 0, sha = 0, copy = 0, skipxx = 0, onethread = 0;

 int iobuf = 1; //default multiplier for I/O buffer is 1, set to 1 MB
 int hashmulti = 20; //default multiplier for hash size on I/O buffer, set to 20 -> 20 MB hashes

 if (argc<2) usage("Not enough parameters!"); // no parameters at all
 if ((strcmp(argv[1],"-h")==0) || (strcasecmp(argv[1],"--help"))==0) usage(NULL);
 if ((strcmp(argv[1],"-V")==0) || (strcasecmp(argv[1],"--version"))==0) version(0);
 if ((strcmp(argv[1],"-l")==0) || (strcasecmp(argv[1],"--licenses"))==0) version(1);

 if (argc<3) usage("Not enough parameters!"); //only 1 parameter and not -h -V or -l

 for (int i = 1; i < argc-2; i++) {
  if ((argv[i][0]=='-') && (strlen(argv[i])>1)) {
   if (argv[i][1]=='-') {
    // parameter starts with --
    if (strcasecmp(argv[1],"--help")==0) usage(NULL);    // Just in case
    if (strcasecmp(argv[1],"--licenses")==0) version(1); //*
    if (strcasecmp(argv[1],"--version")==0) version(0);  //**
    if (strcasecmp(argv[i],"--verbose")==0) { verbose = 1; continue; }
    if (strcasecmp(argv[i],"--dry")==0) { dry = 1; continue; }
    if (strcasecmp(argv[i],"--sha256")==0) { sha = 1; continue; }
    if (strcasecmp(argv[i],"--copy")==0) { copy = 1; continue; }
    if (strcasecmp(argv[i],"--skipxx")==0) { skipxx = 1; continue; }
    if (strcasecmp(argv[i],"--one-thread")==0) { onethread = 1; continue; }
    if (strcasecmp(argv[i],"--io-buffer")==0) {
     if (i+1<argc-2) {
      iobuf = 0;
      i++;
      iobuf = atoi(argv[i]);
      if ((iobuf<=0) || (iobuf>100)) usage("Invalid io-buffer size, must be integer between 1-100!");
     }
    }
    if (strcasecmp(argv[i],"--hash-chunk")==0) {
     if (i+1<argc-2) {
      hashmulti = 0;
      i++;
      hashmulti = atoi(argv[i]);
      if ((hashmulti<=0) || (hashmulti>100)) usage("Invalid hash-chunk multiplier, must be integer between 1-100!");
     }
    }
   }
   else {
    //only one - and after that letters may come
    for (int j = 1; j < strlen(argv[i]); j++) {
     switch (argv[i][j]) {
      case 'h': { usage(NULL); }
      case 'V': { version(0); }
      case 'l': { version(1); }
      case 'v': { verbose = 1; break; }
      case 'n': { dry = 1; break; }
      case 's': { sha = 1; break; }
      case 'c': { copy = 1; break; }
      case 'x': { skipxx = 1; break; }
      case '1': { onethread = 1; break; }
      default: { usage("Unknown option!\n"); }
     }
    }
   }
  } else usage("Unknown option!");
 }

 if (strcmp(argv[argc-2], argv[argc-1])==0) ex_err("Source and destination can't be the same!\n");

 if (verbose) {
  if (iobuf!=1) printf("I/O buffer set to %i MB!\n", iobuf);
  if ((iobuf!=1) || (hashmulti!=20)) printf("Hash size set to %i MB!\n", hashmulti*iobuf);
 }
 BUFSIZE*=iobuf;
 HASHBLOCK=BUFSIZE*hashmulti;

 compare(getFileInfo(argv[argc-2]), getFileInfo(argv[argc-1]), verbose, dry, sha, copy, skipxx, onethread);
 return 0;
}
