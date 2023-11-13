/**************************************************************
 * Class:  CSC-415-0# Fall 2021
 * Names:
 * Student IDs:
 * GitHub Name:
 * Group Name:
 * Project: Basic File System
 *
 * File: fsInit.c
 *
 * Description: Main driver for file system asment.
 *
 * This file is where you will start and initialize your system
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "fsLow.h"  // VCB defined here
#include "mfs.h"

// #include "fsDebug.h"

// Keep VCB in memory
VCB* g_vcb = NULL;

// For writing root directory
struct initRootDirectory {
  directory_entry root;
  directory_entry parentOfRoot;
};

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
  /*
    TODO:
      - Add a check to see if blockSize is a power of 2
      - Instead of writing the root directory, use b_write() instead
  */
  printf("Initializing File System with %ld blocks with a block size of %ld\n",
         numberOfBlocks, blockSize);
  unsigned char* buffer;
  int blocksRead;
  int blocksWritten;
  VCB* vcb;

  // Initialize our buffer that'll copy data and write to disk
  buffer = malloc(MINBLOCKSIZE);
  if (buffer == NULL) {
    printf("Cannot malloc buffer!\n");
    return PART_ERR_INVALID;
  }

  // Read first block after boot sector
  blocksRead = LBAread(buffer, 1, 0);
  if (blocksRead <= 0) {
    printf("Unable to read disk!\n");
    return PART_ERR_INVALID;
  }

  // Load raw bytes and interpret bytes as volume control block
  vcb = (VCB*)buffer;

  // Check if partition has been initialized
  if (vcb->magic_signature == (uint64_t)NULL) {
    printf("Disk is not initialized. Creating a new partition.\n");
  } else if (vcb->magic_signature != VOL_SIGNATURE) {
    printf("Disk signature does not match!\n");
    return PART_ERR_INVALID;
  }

  // Initialize our volume
  vcb->magic_signature = VOL_SIGNATURE;
  vcb->volume_size = blockSize*numberOfBlocks;
  vcb->block_size = blockSize;
  vcb->num_blocks = numberOfBlocks;

  // Calculate minimum blocks needed to address whole volume
  double dBlocksNeeded = numberOfBlocks*sizeof(FAT_block)/((double)(blockSize));
  uint64_t blocksNeeded = ceil(dBlocksNeeded);
  
  // Write FAT table to represent numberOfBlocks blocks
  /*
    Since the block size is fixed, the File Allocation Table is going to have 
    extra blocks at the end that addresses to nothing. When allocating for space in 
    the future, we need to check if that space is actually valid.
    For example, if we have a 10 MB volume with 512 B block size:
      Available Space: 10MB - 128B = 9999872B
      Blocks to represent: 9999872B / 512B = 19531
      Min space for FAT table: 19531 * 4B <-(size of a FAT block) = 78124B
      Blocks of space needed: 78124B / 512B = 152.6 blocks
      Rounding 152.6 up to 153, we get...
      Blocks to represent = 153 * 512 / 4 = 19584 FAT blocks
      Extra FAT blocks that point to disk space we don't have: 19584 - 19531 = 53 blocks
  */ 
  FAT_block* FATTable = (FAT_block*)calloc(blocksNeeded*blockSize, sizeof(FAT_block));

  // FAT table starts at block 1 after the VCB, and takes up FAT_length blocks
  vcb->FAT_start = 1;
  vcb->FAT_length = blocksNeeded;

  // Initialize FAT blocks
  for (int i = 0; i < blocksNeeded*blockSize; i++) {
    FATTable[i].in_use = 0;
    FATTable[i].end_of_file = 0;
    FATTable[i].next_lba_block = 0;
  }

  // Debugging purposes
  // for (int i = 0; i < blocksNeeded*blockSize; i++) {
  //   FATTable[i].in_use = 1;
  //   FATTable[i].end_of_file = 0;
  //   FATTable[i].next_lba_block = 0;
  // }
  // 0x013600;
  // 0x013800;
  
  /*
    Initialize root directory ---------
    In a FAT32 file system, files and directories are stored in the same way.
    We differentiate between files and directories with the is_directory bit.
    When this bit is set, we will read the directory as a sequence of more
    directory_entry blocks. This allows for subdirectories, but also eliminates
    the need for a fixed number of root directories. In FAT32, they store 
    the root directory right at the beginning of the data region.

    Pleae feel free to correct me if I'm wrong 
      - Miguel
    https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
  */

  // Directory entry will start right after FAT
  vcb->DE_start = vcb->FAT_start + vcb->FAT_length;
  vcb->DE_length = 1;

  // Create the root directory    
  directory_entry rootDir;
  rootDir.block_location = vcb->DE_start;
  strncpy(rootDir.name, "/", sizeof(rootDir.name) / sizeof(rootDir.name[0]));          
  rootDir.is_directory = 1;   
  rootDir.file_size = sizeof(directory_entry)*2;    // We are inserting 2 directories into the directory 
  rootDir.date_created = time(0); 
  rootDir.last_modified = time(0); 

  // Create subdirectories of root
  directory_entry parentOfRoot = rootDir; // The parent of the root is itself
  strncpy(parentOfRoot.name, "..", sizeof(parentOfRoot.name) / sizeof(parentOfRoot.name[0]));        

  // Update FAT table to tell it that the first block has been taken
  FATTable[0].in_use = 1;
  FATTable[0].end_of_file = 1;

  // Copy bytes of directory entry to buffer
  struct initRootDirectory initRootDir;
  initRootDir.root = rootDir;
  initRootDir.parentOfRoot = parentOfRoot;

  printf("size of de buff: %ld", sizeof(struct initRootDirectory));
  unsigned char* DEBuffer = malloc(vcb->block_size);
  memset(DEBuffer, '\0', vcb->block_size);
  memcpy(DEBuffer, &initRootDir, sizeof(struct initRootDirectory));

  // Write Root directory
  blocksWritten = LBAwrite(DEBuffer, 1, 154);
  if (blocksWritten < 1) {
    printf("Unable to write to disk!\n");
    return PART_ERR_INVALID;
  }

  // Write FAT
  blocksWritten = LBAwrite(FATTable, blocksNeeded, 1);
  if (blocksWritten < blocksNeeded) {
    printf("Unable to write to disk!\n");
    return PART_ERR_INVALID;
  }

  // Write VCB
  memcpy(buffer, vcb, sizeof(VCB));
  blocksWritten = LBAwrite(buffer, 1, 0);
  if (blocksWritten <= 0) {
    printf("Unable to write to disk!\n");
    return PART_ERR_INVALID;
  }

  // printf("Debuuuuuuuuuuuuuuuuggggggggggg\n");
  // Copy VCB to g_vcb to keep it in memory
  // memcpy(g_vcb, vcb, sizeof(VCB));
  // *g_vcb = *vcb;
  // printf("This is our global VCB in memory: %ld\n", g_vcb->DE_start);

  free(buffer);
  free(FATTable);
  free(DEBuffer);

  return 0;
}

void exitFileSystem() { 
  fs_closedir(g_fs_cwd);
  printf("System exiting\n"); 
}

VCB* getVCB() {
  // Returns the volume control block.

  // Check if initialized
  // if (g_vcb != NULL && force == 0) {
  //   printf("VCB is not null\n");
  //   return g_vcb;
  // } else {
  //   printf("malloc buffer for vcb\n");
  //   g_vcb = (VCB*)malloc(sizeof(VCB));
  //   unsigned char* buffer = malloc(MINBLOCKSIZE);
  //   int blocksRead = LBAread(buffer, 1, 0);
  //   if (blocksRead < 1) {
  //     printf("There was an error reading the VCB\n");
  //     free(buffer);
  //     return NULL;
  //   } else {
  //     printf("Successfully loaded VCB\n");
  //     g_vcb = (VCB*)buffer;
  //     free(buffer);
  //     return g_vcb;
  //   }
  // }

  // Return an instance of VCB. The user has to free it themselves
  // g_vcb = (VCB*)malloc(sizeof(VCB));
  VCB* vcb = malloc(sizeof(VCB));
  unsigned char* buffer = malloc(MINBLOCKSIZE);
  int blocksRead = LBAread(buffer, 1, 0);
  if (blocksRead < 1) {
    printf("There was an error reading the VCB\n");
    free(vcb);
    free(buffer);
    return NULL;
  } else {
    printf("Successfully loaded VCB\n");
    memcpy(vcb, buffer, sizeof(VCB));
    // vcb = (VCB*)buffer;

    // printf("----------------------------------\nPrinting VCB\nmagic_signature: %ld\nvolume_size: %ld\nblock_size: %ld\nnum_blocks: %ld\nFAT_start: %ld\nFAT_length: %ld\nDE_start: %ld\nDE_length: %ld\n----------------------------------\n", 
    //   vcb->magic_signature,
    //   vcb->volume_size,
    //   vcb->block_size,
    //   vcb->num_blocks,
    //   vcb->FAT_start,
    //   vcb->FAT_length,
    //   vcb->DE_start,
    //   vcb->DE_length);

    free(buffer);
    return vcb;
  }
}