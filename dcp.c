#include <stdio.h> //printf()
#include <stdlib.h> //exit()
#include <string.h> //strcmp()

#include "defs.h"
#include "dcp.h"
#include "file.h"
#include "compare.h"

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
 printf("Options:\n");
 printf("-h, --help		show this message and exit\n");
 printf("-l, --licenses		display licensing information and exit\n");
 printf("-V, --version		show version information and exit\n");
 printf("-v, --verbose		verbose mode\n");
 printf("-n, --dry		dry-run, no-opt mode\n");
 printf("-s, --sha256		calculate (and display) SHA256 hash after copy\n");
 printf("-c, --copy		always copy the whole while if differs\n");
 printf("-x, --skipxx		skip the xxHash on the whole file, just do xxHash tables\n");
 printf("-1, --one-thread	use only one thread while computing the chunks' hashes\n");
 if (errmsg) exit(EXIT_FAILURE);
 exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
 int verbose = 0, dry = 0, sha = 0, copy = 0, skipxx = 0, onethread = 0;

 if (argc<2) usage("Not enough parameters!");
 if ((strcmp(argv[1],"-h")==0) || (strcmp(argv[1],"--help"))==0) usage(NULL);
 if ((strcmp(argv[1],"-V")==0) || (strcmp(argv[1],"--version"))==0) version(0);
 if ((strcmp(argv[1],"-l")==0) || (strcmp(argv[1],"--licenses"))==0) version(1);

 if (argc<3) usage("Not enough parameters!");
 for (int i = 1; i < argc-2; i++) {
  if ((strcmp(argv[i],"-v")==0) || (strcmp(argv[i],"--verbose"))==0) {
   verbose = 1;
   continue;
  }
  if ((strcmp(argv[i],"-n")==0) || (strcmp(argv[i],"--dry"))==0) {
   dry = 1;
   continue;
  }
  if ((strcmp(argv[i],"-s")==0) || (strcmp(argv[i],"--sha256"))==0) {
   sha = 1;
   continue;
  }
  if ((strcmp(argv[i],"-c")==0) || (strcmp(argv[i],"--copy"))==0) {
   copy = 1;
   continue;
  }
  if ((strcmp(argv[i],"-x")==0) || (strcmp(argv[i],"--skipxx"))==0) {
   skipxx = 1;
   continue;
  }
  if ((strcmp(argv[i],"-1")==0) || (strcmp(argv[i],"--one-thread"))==0) {
   onethread = 1;
   continue;
  }
  usage("Unkown option!");
 }

 if (strcmp(argv[argc-2], argv[argc-1])==0) ex_err("Source and destination can't be the same!\n");

 compare(getFileInfo(argv[argc-2]), getFileInfo(argv[argc-1]), verbose, dry, sha, copy, skipxx, onethread);
 return 0;
}
