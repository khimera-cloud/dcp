typedef struct workFile {
 //-1 if file can't be stat'ed
 //0 for unknown
 //1 for block
 //2 for normal
 char *name;
 int type;
 unsigned long size;
} workFile;

workFile getFileInfo(char* fn);

//cut every 3 times digits and apply GB, MB, etc on the end
char* humanSize(unsigned long bytes);
