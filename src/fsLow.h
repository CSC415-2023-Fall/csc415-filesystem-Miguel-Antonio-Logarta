/**************************************************************
 * Class:  CSC-415
 * Name: Professor Bierman
 * Student ID: N/A
 * Project: Basic File System
 *
 * File: fsLow.h
 *
 * Description: This file provides the ability to read and write
 *   Logical Blocks and is the main interface for the file system
 *   project.
 *   Note that the layer uses one more block than that presented
 *   to the file organization module.  This block is used to
 *   hold the partition information and is not accessible from
 *   any other layer.  But, when you use the hexdump utility
 *   you will see that the first block is not part of the volume.
 *
 *   The file created by this layer represents the physical hard
 *	drive.  It presents to the logical layer (your layer) as just
 *	a logical block array (a series of blocks - nominally 512 bytes,
 *	that the logical layer can utilize).
 *
 *	It is imperative that the logical layer (your layer) first
 *	call startPartitionSystem before using any function and when
 *	finished calls closePartitionSystem() to ensure that the
 *	file that represents the physical drive is properally closed.
 *
 **************************************************************/
//
// Start Partition System
//
// This is the first function to call before your filesystem starts
// If the filename already exists, then the input values stored in
// volSize and blockSize are ignored.  If the file does not exist, it will
// be created to the specified volume size in units of the block size
// (must be power of 2) plus one for the partition header.
//
// On return
// 		return value 0 = success;
//		return value -1 = file exists but can not open for write
//		return value -2 = insufficient space for the volume
//		volSize will be filled with the volume size
//		blockSize will be filled with the block size

#ifndef _FS_LOW_H
#define _FS_LOW_H

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif
typedef unsigned long long ull_t;

// This struct is exactly 64 bytes
typedef struct VCB_s {
	uint64_t magic_signature;
  uint64_t volume_size;
  uint64_t block_size;
  uint64_t num_blocks;
  uint64_t FAT_start;
  uint64_t FAT_length;
  uint64_t DE_start;
  uint64_t DE_length;
} VCB;

// This struct is exactly 32 bits/4 bytes
// 30 bits allows us to address up to 2^30 lba blocks
typedef struct FAT_block_s {
  unsigned int in_use: 1;
  unsigned int end_of_file: 1;
  unsigned int next_lba_block: 30;
} FAT_block;

// This is exactly 64 bytes
/*
  In a FAT file system, files and directories are stored in the same way.
  The only way to differentiate them is with the is_directory bit.
  If is_directory == 0, read it like a normal file
  Else if is_directory == 1, treat the directory entry's associated lba
    block like a sequence of more directory entries
*/ 
typedef struct directory_entry_s {
  unsigned char is_directory;   // 1 byte. We have 8 bits, so maybe we can use this to set attributes for later
  char name[31];          // 31 bytes
  uint64_t block_location;  // 8 bytes
  size_t file_size;     // 8 bytes
  time_t date_created; // 8 bytes
  time_t last_modified; // 8 bytes
} directory_entry;

// Unitialized global variables that store our state
extern VCB* g_vcb;

int createPartition(char *filename, uint64_t *volSize, uint64_t *blockSize);

int startPartitionSystem(char *filename, uint64_t *volSize,
                         uint64_t *blockSize);

int closePartitionSystem();

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize);
void exitFileSystem();

// Function LBAwrite will write lbaCount BLOCKS from the buffer starting from
// lbaPosition Block. It will return the number of blocks written.
uint64_t LBAwrite(void *buffer, uint64_t lbaCount, uint64_t lbaPosition);

// Function LBAread will read lbaCount BLOCKS into the buffer starting from
// lbaPosition Block. It will return the number of blocks read.
uint64_t LBAread(void *buffer, uint64_t lbaCount, uint64_t lbaPosition);

void runFSLowTest(); // Do not use this, for testing only

VCB* getVCB();

#define MINBLOCKSIZE 512
#define PART_SIGNATURE 0x526F626572742042
#define PART_SIGNATURE2 0x4220747265626F52

#define VOL_SIGNATURE 0x007874654275696C

#define PART_CAPTION                                                           \
  "CSC-415 - Operating Systems File System Partition Header\n\n"

#define PART_ACTIVE 1
#define PART_INACTIVE 0

#define PART_NOERROR 0
#define PART_ERR_INVALID -4

#endif