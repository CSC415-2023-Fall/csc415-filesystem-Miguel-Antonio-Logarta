/**************************************************************
 * Class:  CSC-415-0# Fall 2021
 * Names:
 * Student IDs:
 * GitHub Name:
 * Group Name:
 * Project: Basic File System
 *
 * File: b_io.c
 *
 * Description: Basic File System - Key File I/O Operations
 *
 **************************************************************/

#include "b_io.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "fsLow.h"
#include <sys/types.h>
#include <unistd.h>
#include "mfs.h"

#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512


typedef struct b_fcb {
  /** TODO add al the information you need in the file control block **/
  char *buf;  // holds the open file buffer
  int index;  // holds the current position in the buffer
  int buflen; // holds how many valid bytes are in the buffer
  int location;
  int currentBlock;
  int fileSize;
  int flag;
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; // Indicates that this has not been initialized

// Method to initialize our file system
void b_init() {
  // init fcbArray to all free
  for (int i = 0; i < MAXFCBS; i++) {
    fcbArray[i].buf = NULL; // indicates a free fcbArray
  }

  startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB() {
  for (int i = 0; i < MAXFCBS; i++) {
    if (fcbArray[i].buf == NULL) {
      return i; // Not thread safe (But do not worry about it for this
                // assignment)
    }
  }
  return (-1); // all in use
}

//return file size in blocks
int getFileSize(int location) {
    VCB *vcb = g_vcb;
    int blockSize = sizeof(FAT_block);
    FAT_block *currentBlock = malloc(blockSize);
    char *buffer = malloc(blockSize);

    if (!currentBlock || !buffer) {
        free(currentBlock); // Safe to call free on NULL
        free(buffer);
        return -1; // Indicate failure
    }

    LBAread(buffer, 1, location);
    memcpy(currentBlock, buffer, blockSize);
    int counter = 0;
    while (currentBlock->end_of_file != 1) {
        counter += vcb->block_size;
        LBAread(buffer, 1, currentBlock->next_lba_block);
        memcpy(currentBlock, buffer, blockSize);
    }
    if (currentBlock->next_lba_block > 0) {
        counter += currentBlock->next_lba_block;
    }

    free(currentBlock);
    free(buffer);
    return counter;
}


int createFile(char* pathName) {
    fdDir *fdD;
    VCB* vcb = g_vcb;
    char * fileName = malloc(100);
    char *dirBuffer = malloc(vcb->block_size);
    if (!dirBuffer) {
        printf("Memory allocation failed for dirBuffer\n");
        return -1; // Indicate failure
    }

    if (strchr(pathName, '/') != NULL) {
      
      // Copy the pathName to a new buffer so it can be modified      
      char *pathCopy = strdup(pathName);
      if (!pathCopy) {
          printf("Memory allocation failed for pathCopy\n");
          free(dirBuffer);
          return -1; // Indicate failure
      }

      // Find the last occurrence of '/'
      char *lastSlash = strrchr(pathCopy, '/');
      if (lastSlash != NULL) {
          if (lastSlash != pathCopy) {  // Check if it's not the root directory
              *lastSlash = '\0';  // Cut the path at the last '/'
          } else {
              // If it's the root directory, keep it as is
              strcpy(fileName, lastSlash+1);
              *(lastSlash + 1) = '\0';
          }
          fdD = fs_opendir(pathCopy);
          free(pathCopy); // Free the path copy after use
      }
    }
    else {
        fdD = g_fs_cwd;
        printf("Curren Directory: %s", fdD->directory->name);
    }
    if (fdD == NULL) {  
        printf("Path not found\n");
        free(dirBuffer);
        return -1; // Indicate failure
    }

    LBAread(dirBuffer, 1, fdD->directory->block_location);
    directory_entry *DE = malloc(sizeof(directory_entry));
    
    if (!DE) {
        printf("Memory allocation failed for directory entry\n");
        free(dirBuffer);
        return -1; // Indicate failure
    }

    //setting uo the DE
    DE->block_location = UseNextFreeBlock(NULL);
    DE->is_directory=0;
    DE->file_size=0;
    time_t currentTime;
    time(&currentTime); // Get the current time
    DE->date_created = currentTime;
    DE->last_modified = currentTime;
    strcpy(DE->name, fileName);
    
    //copy the new DE into its parent Direcotry
    memcpy(dirBuffer+fdD->directory->file_size, DE, sizeof(directory_entry));
    LBAwrite(dirBuffer, 1, fdD->directory->block_location);
    
    free(dirBuffer);
    free(DE);
    free(fileName);

    return 0; // Assuming 0 indicates success
}


// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, O_CREAT, or O_RDWR
b_io_fd b_open(char *filename, int flags) {
    if (startup == 0) {
        b_init();  // Initialize our file system if not already done
    }

    b_io_fd returnFd = b_getFCB();  // Get a free File Control Block
    if (returnFd == -1) {
        printf("No free File Control Block available");
        return -1;
    }

    // Allocate a buffer for this file
    fcbArray[returnFd].buf = (char *)malloc(B_CHUNK_SIZE);
    if (fcbArray[returnFd].buf == NULL) {
        printf("Memory allocation for buffer failed");
        return -1;
    }

    // Initialize fields of the file control block
    fcbArray[returnFd].buflen = 0;
    fcbArray[returnFd].flag = flags;
    fcbArray[returnFd].index = 0;

    

    // Additional logic for handling file open based on flags
    if (flags == O_WRONLY | O_CREAT) {
        // If the file is opened in write-only or create mode, handle as needed
        createFile(filename);
    } 
        // Read mode: Retrieve and store file information in the file control block
        // Open the directory and check if the file exists
        fdDir *fdD = fs_opendir(filename);
        if (fdD == NULL) {
            printf("Path/file does not exist\n");
            free(fcbArray[returnFd].buf); // Free allocated buffer
            return -1;

        }
        directory_entry* DE = malloc(sizeof(directory_entry));
        if (!DE) {
            printf("Memory allocation failed for directory entry\n");
            free(fcbArray[returnFd].buf);
            return -1;
        }

        memcpy(DE, fdD->dirEntryPosition, sizeof(directory_entry));
        fcbArray[returnFd].location = DE->block_location;
        fcbArray[returnFd].fileSize = DE->file_size;
        fcbArray[returnFd].currentBlock = fcbArray[returnFd].location;
        
        free(DE); // Free the directory entry after use
    

    return returnFd; // Return the file descriptor
}



/*
//uses and returns the next freeblock
int UseNextFreeBlock(int previousBlock){
  VCB * vcb= g_vcb;
  int FAT_BLOCK_location; //current FAT_BLOCK struct location (in blocks) in FAT table 
  int FAT_BLOCK_index; //the index of the FAT_BLOCK struct(in bytes) in FAT_BLOCK_location block
  char* BlockBuffer = malloc(sizeof(vcb->block_size));
  FAT_block* modifiedBlock = malloc(sizeof(FAT_block));
  FAT_block* freeFatBlock = freeSpaceList;
  freeSpaceList = freeSpaceList->next_lba_block;
  freeFatBlock->in_use= 1;
  int numberOfFATBlocksInBlock = vcb->block_size / sizeof(FAT_block);
  FAT_BLOCK_location = vcb->FAT_start +firstFreeBlock / numberOfFATBlocksInBlock;
  FAT_BLOCK_index = firstFreeBlock % numberOfFATBlocksInBlock;
  LBAread(BlockBuffer,1,FAT_BLOCK_location);
  memcpy(modifiedBlock, BlockBuffer+FAT_BLOCK_index, sizeof(FAT_block));
  modifiedBlock->in_use=1;
  memcpy(BlockBuffer+FAT_BLOCK_index, modifiedBlock, sizeof(FAT_block));
  LBAwrite(BlockBuffer,1,FAT_BLOCK_location);
  int returnFirstFreeBlock= firstFreeBlock;
  firstFreeBlock = freeFatBlock->next_lba_block;
  
  return returnFirstFreeBlock;
}
*/

int UseNextFreeBlock(int previousBlock) {
    VCB *vcb = g_vcb;
    int FAT_BLOCK_location; // current FAT_BLOCK struct location (in blocks) in FAT table 
    int FAT_BLOCK_index; // the index of the FAT_BLOCK struct (in bytes) in FAT_BLOCK_location block
    char* BlockBuffer = malloc(vcb->block_size);
    if (BlockBuffer == NULL) {
        return -1; // Memory allocation failed
    }
    FAT_block* modifiedBlock = malloc(sizeof(FAT_block));
    if (modifiedBlock == NULL) {
        free(BlockBuffer);
        return -1; // Memory allocation failed
    }
    FAT_block* freeFatBlock = freeSpaceList;
    freeSpaceList = freeSpaceList->next_lba_block;
    freeFatBlock->in_use = 1;

    // Update the previous block
    if (previousBlock != -1) { // Check if there is a valid previous block
        int numberOfFATBlocksInBlock = vcb->block_size / sizeof(FAT_block);
        int prevBlockLocation = vcb->FAT_start + previousBlock / numberOfFATBlocksInBlock;
        int prevBlockIndex = previousBlock % numberOfFATBlocksInBlock;

        LBAread(BlockBuffer, 1, prevBlockLocation);
        FAT_block* previousFatBlock = (FAT_block*)(BlockBuffer + prevBlockIndex * sizeof(FAT_block));
        previousFatBlock->next_lba_block = firstFreeBlock; // Point to the new block
        LBAwrite(BlockBuffer, 1, prevBlockLocation); // Write back the updated block
    }

    // Allocate the new block
    int numberOfFATBlocksInBlock = vcb->block_size / sizeof(FAT_block);
    FAT_BLOCK_location = vcb->FAT_start + firstFreeBlock / numberOfFATBlocksInBlock;
    FAT_BLOCK_index = firstFreeBlock % numberOfFATBlocksInBlock;
    LBAread(BlockBuffer, 1, FAT_BLOCK_location);
    memcpy(modifiedBlock, BlockBuffer + FAT_BLOCK_index * sizeof(FAT_block), sizeof(FAT_block));
    modifiedBlock->in_use = 1;
    memcpy(BlockBuffer + FAT_BLOCK_index * sizeof(FAT_block), modifiedBlock, sizeof(FAT_block));
    LBAwrite(BlockBuffer, 1, FAT_BLOCK_location);

    int returnFirstFreeBlock = firstFreeBlock;
    firstFreeBlock = freeFatBlock->next_lba_block;

    free(BlockBuffer);
    free(modifiedBlock);

    return returnFirstFreeBlock;
}


int getLastBlockInFile(location){

  VCB * vcb = g_vcb;
  int blockSize = sizeof(FAT_block);
  FAT_block * currentBlock = malloc(blockSize);
  currentBlock =freeSpaceList;
  int returnBlockNum = location;
  char *buffer = malloc(blockSize);
  LBAread(buffer, 1, location);
  memcpy(currentBlock, buffer, blockSize);
  int counter = 0;
  while(currentBlock->end_of_file != 1){
    returnBlockNum = currentBlock->next_lba_block;
    counter += vcb->block_size;
    LBAread(buffer, 1, currentBlock->next_lba_block);
    memcpy(currentBlock, buffer, blockSize);
  }
  return returnBlockNum;
}

// Interface to write function
int b_write(b_io_fd fd, char *buffer, int count) {
  if (startup == 0)
    b_init(); // Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    return (-1); // invalid file descriptor
  }
  
  VCB *vcb = g_vcb;
  int parBufferIndex = 0;
  char* fileBlockBuffer = malloc(vcb->block_size); 
  int bytesToRead;
  int remainingBytes = fcbArray[fd].fileSize %vcb->block_size;
  if (fcbArray[fd].fileSize < vcb->block_size || fcbArray[fd].fileSize %vcb->block_size >0){
    LBAread(fileBlockBuffer, 1, fcbArray[fd].currentBlock);
    memcpy(fileBlockBuffer, buffer, remainingBytes);
    parBufferIndex +=remainingBytes;
    LBAwrite(fileBlockBuffer, 1, fcbArray[fd].currentBlock);
    count = count-remainingBytes;
  }
   
  while(count> vcb->block_size){
    fcbArray[fd].currentBlock = UseNextFreeBlock(fcbArray[fd].currentBlock);
    bytesToRead = count < vcb->block_size ? count : vcb->block_size;
    memcpy(fileBlockBuffer, buffer, fcbArray[fd].currentBlock);
    LBAwrite(buffer + parBufferIndex, 1, fcbArray[fd].currentBlock);
    count -=vcb->block_size;
  }
  if (count>0){
    fcbArray[fd].currentBlock = UseNextFreeBlock(fcbArray[fd].currentBlock);
    bytesToRead = count < vcb->block_size ? count : vcb->block_size;
    memcpy(fileBlockBuffer, buffer, fcbArray[fd].currentBlock);
    LBAwrite(buffer + parBufferIndex, 1, fcbArray[fd].currentBlock);
  }

  return (0); // Change this
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be
// enough Part 2 is after using what was left in our buffer there is still 1 or
// more block
//        size chunks needed to fill the callers request.  This represents the
//        number of bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the
// callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from
//        a refill of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char *buffer, int count) {
  if (startup == 0)
    b_init(); // Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    return (-1); // invalid file descriptor
  }

  b_fcb *fcb = &fcbArray[fd];
  if (fcb->flag == O_WRONLY){
    printf("Cant read file, flag is set to O_WRONLY");
    return (-1);
  }

  VCB * vcb= g_vcb;
  int FAT_BLOCK_location; //current FAT_BLOCK struct location (in blocks) in FAT table 
  int FAT_BLOCK_index; //the index of the FAT_BLOCK struct(in bytes) in FAT_BLOCK_location block
  char * FAT_block_buffer = malloc(sizeof(FAT_block));
  FAT_block* fatBlock = malloc(sizeof(FAT_block));
  int bytesRead;
  int bytesToreturn;
  int part1=0, part2=0, part3=0;
  int remainingBytesInBuffer = fcb->buflen - fcb->index;
  int amountAlreadyDelivered = fcb->currentBlock * B_CHUNK_SIZE - remainingBytesInBuffer;

  if ((amountAlreadyDelivered + count) > fcb->fileSize){
    count = fcb->fileSize - amountAlreadyDelivered;
  }

  if(remainingBytesInBuffer >= count){
    part1 = count; 
  }
  else {
    part1= remainingBytesInBuffer;
  }

  int numberOfBlocksToCopy = (count - remainingBytesInBuffer) / B_CHUNK_SIZE;
  part2 = numberOfBlocksToCopy *B_CHUNK_SIZE;

  part3 = count - remainingBytesInBuffer - part2;

  if (part1 >0){
    memcpy(buffer, fcb->buf + fcb->index, part1);
    fcb->index+=part1;
  }
  if (part2 > 0){
    
    for (int i =0 ; i<numberOfBlocksToCopy; i++){
      //read file block
      bytesRead += LBAread(buffer + part1, 1, fcb->currentBlock);

      //read FAT_BLOCK struct
      //calculate which block the struct is located in the FAT table
      FAT_BLOCK_location = vcb->FAT_start + fcb->currentBlock / sizeof(FAT_block);
      FAT_BLOCK_index = fcb->currentBlock % sizeof(FAT_block);
      LBAread(FAT_block_buffer, 1, FAT_BLOCK_location);
      fatBlock = (FAT_block*)FAT_block_buffer;
      fcb->currentBlock = fatBlock->next_lba_block;
    }
   
    part2 = bytesRead;
  }
  if (part3 >0){
    bytesRead = LBAread(fcb->buf, 1, fcb->currentBlock + fcb->location);
    
    //read FAT_BLOCK struct
    //calculate which block the struct is located in the FAT table
    FAT_BLOCK_location = vcb->FAT_start + fcb->currentBlock / sizeof(FAT_block);
    FAT_BLOCK_index = fcb->currentBlock % sizeof(FAT_block);
    LBAread(FAT_block_buffer, 1, FAT_BLOCK_location);
    fatBlock = (FAT_block*)FAT_block_buffer;
    fcb->currentBlock = fatBlock->next_lba_block;

    fcb->index=0;
    fcb->buflen = bytesRead;
  }

  if (bytesRead < part3){
    part3= bytesRead;
  }

  if (part3 > 0){
    memcpy(buffer+part1+part2, fcb->buf +fcb->index, part3);
    fcb->index+=part3;
  }

  return (part1+part2+part3); 
}

// Interface to Close the file
int b_close(b_io_fd fd) {}
