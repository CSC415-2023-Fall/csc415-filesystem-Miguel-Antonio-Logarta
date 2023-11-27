/**************************************************************
* Class:  CSC-415
* Name: Miguel Logarta
* Student ID: N/A
* Project: Basic File System
*
* File: partition.h
*
* Description: 
*	An interface for manipulating the volume control block (VCB)
* and the File Allocation Table (FAT) located at the beginning
* of our volume partition.
*
**************************************************************/

#ifndef _PARTITION_H
#define _PARTITION_H

#include <time.h>
#include <stdio.h>

#include "fsLow.h"

/* 
	Volume Control Block: 64 bytes
	This struct is responsible for storing information about a partition
*/
#pragma pack (1)
typedef struct VCB_s {
	uint64_t magic_signature;   // Unique number to identify the partition
  uint64_t volume_size;       // Size of volume in bytes
  uint64_t block_size;        // Minimum block size of partition in bytes
  uint64_t num_blocks;        // Total number of blocks in partition
  uint64_t FAT_start;         // Starting lba position of File Allocation Table
  uint64_t FAT_length;        // Number of lba blocks the File Allocation Tables takes
  uint64_t DE_start;          // Starting lba block of our root directory
  uint64_t DE_length;         // Number of lba blocks our root directory takes up
} VCB;

/* 
	File Allocation Table Block: 4 bytes
	This struct holds information for each block of space in our file system
	Address is 30 bits long, which allows us to address up to 2^30 lba blocks
*/
#pragma pack (1)
typedef struct FAT_block_s {
  unsigned int in_use: 1;
  unsigned int end_of_file: 1;
  unsigned int next_lba_block: 30;
} FAT_block;

/*
	Directory Entry: 64 bytes
	This struct is how we store files and subdirectories in the file system.
  In a FAT file system, files and directories are stored in the same way.
  The only way to differentiate them is with the is_directory bit.
  If is_directory == 0, read it like a normal file
  Else if is_directory == 1, treat the directory entry's associated lba
    block like a sequence of more directory entries
*/ 
#pragma pack (1)
typedef struct directory_entry_s {
  unsigned char is_directory;   // We have 8 bits, so maybe we can use this to set attributes for later
  char name[31];
  uint64_t block_location;
  size_t file_size;
  time_t date_created;
  time_t last_modified;
} directory_entry;

typedef unsigned int lba_block;

/* 
	Global variables to hold vcb and FAT in memory. 
	Holding a cached state eliminates overhead of reading the
		vcb and FAT from the disk.
*/
extern VCB *g_vcb;
extern FAT_block *g_FAT;

/* Volume Partition Control */
VCB *fs_getvcb();
VCB *fs_writevcb(VCB* vcb);
void fs_freevcb(VCB* vcb);
FAT_block *fs_getFAT();
FAT_block *fs_writeFAT(FAT_block* fat, uint64_t numBlocks);
void fs_freefat(FAT_block* fat);
lba_block findFreeBlock();


#endif