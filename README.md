# dcp
Delta Copy for Linux

This program uses the great [xxHash](https://github.com/Cyan4973/xxHash) algorithm to copy only the differences between two files on a Linux system.
It works well with large to extremly large files, for example copying a disk onto another.
Useful for making incremental full disk (block-level) backups.

It breaks down the source and destination files to smaller chunks and uses multiple threads to calculate
the (xx)hash of these chunks. Then only those that differ will be copied over.
Both files will be fully read (2x file I/O), but only differences will be copied (0%<write I/O<100%).
If only a few bytes differ in a very large file, *this is the way*.

Currently it's tested only on Linux and useful only on a local system.
Also comes a small utility dcp-cbr, which can change n bytes in a file to random values, for testing dcp.

## licenses

> dcp version 0.2, GPLv3
> https://github.com/khimera.cloud/dcp
>
> Additional licenses:
> xxHash by Yann Collet - https://cyan4973.github.io/xxHash/
> OpenSSL Project - https://openssl.org/
> GNU C Library - https://gnu.org/software/libc/

## usage

> Usage: dcp [OPTION]... <src dev/file> <dest dev/file>
> Differential copy tool using multithreaded xxHash
>
> Options:
> -h, --help		show this message and exit
> -l, --licenses		display licensing information and exit
> -V, --version		show version information and exit
> -v, --verbose		verbose mode
> -n, --dry		dry-run, no-opt mode
> -s, --sha256		calculate (and display) SHA256 hash after copy
> -c, --copy		always copy the whole while if differs
> -x, --skipxx		skip the xxHash on the whole file, just do xxHash tables
> -1, --one-thread	use only one thread while computing the chunks' hashes

## dcp-cbr

cbr stands for Change Byte Randomly, no motorcycles involved ;)
It takes 1 parameter as a filename and optionally another for the number of runs (default is 1).

> DCP Change Byte Randomly version 0.2, GPLv3
> https://github.com/khimera.cloud/dcp
>
> Usage: dcp-cbr <filename> <#times (default is 1)>
