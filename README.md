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
