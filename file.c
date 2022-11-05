#define _GNU_SOURCE //needed by asprintf() before including stdio.h
#include <stdio.h> //asprintf()
#include <fcntl.h> //open()
#include <sys/stat.h> //stat()
#include <sys/ioctl.h> //ioctl()
#include <unistd.h> //close()
#include <stdlib.h> //malloc()
#include <linux/fs.h> //BLKGETSIZE64

#include "file.h"

workFile getFileInfo(char* fn) {
 workFile ret;
 struct stat statbuf;

 ret.name = fn;
 ret.type = -1;
 ret.size = 0;

 //return -1 if can't be stat'ed (e.g. doess't exist)
 if (stat(fn, &statbuf)!=0) return ret;
 ret.type = 0;
 //If block device, try to read size with ioctl
 if (S_ISBLK(statbuf.st_mode)) {
   ret.type = 1;
   int fd = open(fn, O_RDONLY);
   if (fd==-1) return ret;
   if (ioctl(fd, BLKGETSIZE64, &ret.size)==-1) return ret;
   close(fd);
 }
 //If regular file, we already have the size from stat()
 if (S_ISREG(statbuf.st_mode)) {
   ret.type = 2;
   ret.size = statbuf.st_size;
 }
 return ret;
}

char* humanSize(unsigned long bytes) {
 char* ret;

 //count number of digits in bytes
 unsigned long n=bytes;
 int count=0;
 while (n!=0) {
  n/=10;
  count++;
 }

 //If zero
 if (count==0) {
  asprintf(&ret, "0");
  return ret;
 }

 // cut to human size - YES, I know that bytes should be 1024 not 1000,
 // it's easier this way, let's say it's SI :)
 unsigned long cutsize = bytes;
 while (cutsize>=1000) cutsize/=1000;

 // set trailing character
 char lastc = 'B'; //Bytes by default
 if (count > 3) lastc = 'K';
 if (count > 6) lastc = 'M';
 if (count > 9) lastc = 'G';
 if (count > 12) lastc = 'T';
 if (count > 15) lastc = 'P';

 //Exabyte? XD - seriously, this function dies at Exabyte length...

 if (lastc == 'B') {
  asprintf(&ret, "%lu%c", cutsize, lastc);
 } else {
  asprintf(&ret, "%lu%cB", cutsize, lastc);
 }
 return ret;
}
