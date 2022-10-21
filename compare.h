void compare(workFile srct, workFile dstt, int verbose, int dry, int sha, int copy, int skipxx);

//src file name, dst file name
//offset to start in src and dst (0 for beginning)
//lenght of bytes to copy, set to src filesize to copy whole file
//returns number of bytes copied
unsigned long copyBytes(char* src, char* dst, \
			unsigned long offset, unsigned long length, \
			int verbose);

void calcBothSha(char* src, char* dst, unsigned long until, int verbose);
