#include <stdio.h> //printf()
#include <stdlib.h> //exit()
#include <string.h> //strcmp()
#include <time.h> //time() for srand()

#include "defs.h"
#include "file.h"

int main (int argc, char* argv[]) {
 printf("DCP Change Byte Randomly version %s, GPLv3\n", VERSION);
 printf("%s\n\n", URL);

 if (argc<2) {
  printf("Usage: dcp-cbr <filename> <#times (default is 1)>\n");
  exit(EXIT_FAILURE);
 }

 printf("Running DCP-CBR on file %s!\n", argv[1]);

 workFile wf = getFileInfo(argv[1]);

 unsigned cycles = 1;
 if (argc>=3) cycles = atoi(argv[2]);

 if (wf.type==-1) printf("File can't be opened, ");
 if (wf.type==0) printf("File type unknown, ");
 if (wf.type==1) printf("File type is block, ");
 if (wf.type==2) printf("File type is regular, ");
 char* hs = humanSize(wf.size);
 printf("size is %lu bytes (%s)\n", wf.size, hs);
 free(hs);
 if (wf.type<=0) ex_err("File is neither block or regular!\n");

 //The random generator changes state every second, good enough for testing
 srand(time(NULL));
 FILE* file;
 unsigned long r;
 unsigned char c;
 file = fopen(argv[1], "r+b");

 for (unsigned i=0; i<cycles; i++) {
  r = rand();
  while (r >= wf.size) r=rand();
  c = rand();

  printf("#%u Random byte position is: %lu, byte data is %02x\n", i, r, c); //IRC!!!

 // fseek(file, r, SEEK_SET);
  if (fseek(file, r, SEEK_SET)<0) { perror("fseek"); exit(EXIT_FAILURE); }
  fwrite(&c, 1, 1, file);
 }
 fclose(file);

 return 0;
}
