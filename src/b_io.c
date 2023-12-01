/**************************************************************
* Class:  CSC-415-02 Fall 2023
* Names: Mohammed Deeb, Miguel Antonio Logarta, Abdarrahman Ayyaz, Avitaj Singh Bhangoo
* Student IDs: 922875899, 923062683, 922859909, 921303783
* GitHub Name:
* Group Name: Byte Builders
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
#include "partition.h"

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
  int DELocation;
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


struct fs_diriteminfo *findFile(char *pathName) {
    VCB *vcb = g_vcb;
    if (pathName == NULL) {
        printf("Invalid path name provided\n");
        return NULL;
    }

    char *fileName = strrchr(pathName, '/');
    if (fileName == NULL) {
        printf("Invalid path format\n");
        return NULL;
    }

    fileName++; // Skip the '/' to get the file name

    char *pathCopy = strdup(pathName);
    if (!pathCopy) {
        printf("Memory allocation failed for pathCopy\n");
        return NULL;
    }

    char *lastSlash = strrchr(pathCopy, '/');
    if (lastSlash != NULL) {
        *lastSlash = '\0';  // Cut the path at the last '/'
    }
    /*
    char* dirBuff = malloc(sizeof(vcb->block_size));
    fdDir *fdD = fs_opendir(lastSlash ? pathCopy : "/");
    LBARead(,fdD->directory->block_location);
    */
  
 
    fdDir *fdD = fs_opendir(lastSlash ? pathCopy : "/");
    free(pathCopy);
    if (fdD == NULL) {
        printf("Path not found\n");
        return NULL;
    }

    struct fs_diriteminfo *fs_dir;
    while ((fs_dir = fs_readdir(fdD)) != NULL) {
        if (strcmp(fs_dir->d_name, fileName) == 0) {
            break;  // File found
        }
    }
  
    fs_closedir(fdD);
    return fs_dir;  // fs_dir is NULL if the file was not found
}

int createFile(char* pathName) {
    if (!pathName) {
        printf("Invalid path name provided\n");
        return -1;
    }

    VCB* vcb = fs_getvcb();
    // printf("block size = %d\n", vcb->block_size);
    char *dirBuffer = (char*)malloc(vcb->block_size);
    if (!dirBuffer) {
        printf("Memory allocation failed for dirBuffer\n");
        return -1;
    }
 
    // Allocate memory for DE before using it
    directory_entry *DE = malloc(sizeof(directory_entry));
    if (!DE) {
        printf("Memory allocation failed for directory entry\n");
        free(dirBuffer);
        return -1;
    }

    char *lastSlash = strrchr(pathName, '/');
    if (lastSlash == NULL || *(lastSlash + 1) == '\0') {
        printf("Invalid path format or empty file name\n");
        free(dirBuffer);
        free(DE);
        return -1;
    }

    char *fileName = lastSlash + 1;
    if (strlen(fileName) >= sizeof(DE->name)) {
        printf("File name too long\n");
        free(dirBuffer);
        free(DE);
        return -1;
    }

    strncpy(DE->name, fileName, sizeof(DE->name) - 1);  // Ensure null-termination
    DE->name[sizeof(DE->name) - 1] = '\0';

    // Create a copy of the path to modify it
    char *pathCopy = strndup(pathName, lastSlash - pathName);
    if (!pathCopy) {
        printf("Memory allocation failed for pathCopy\n");
        free(dirBuffer);
        free(DE);
        return -1;
    }
    //if the file created at root
    if (strcmp(pathCopy, "") == 0) {
      pathCopy[0] = '/';
      pathCopy[1] = '\0';
    }

    fdDir *fdD = fs_opendir(pathCopy);
  
    if (fdD == NULL) {
        printf("Directory not found\n");
        free(dirBuffer);
        free(DE);
        free(pathCopy);
        return -1;
    }

    LBAread(dirBuffer, 1, fdD->directory->block_location);

    DE->block_location = UseNextFreeBlock(-1);
    DE->is_directory = 0;
    DE->file_size = 0;
    time_t currentTime;
    time(&currentTime);
    DE->date_created = currentTime;
    DE->last_modified = currentTime;

    // Copy the new DE into its parent Directory
    memcpy(dirBuffer + fdD->directory->file_size, DE, sizeof(directory_entry));
    fdD->directory->file_size += sizeof(directory_entry);
    
    //modify ParentDr size
    directory_entry *parentDE = malloc(sizeof(directory_entry));
    memcpy(parentDE, dirBuffer, sizeof(directory_entry));
    parentDE->file_size = fdD->directory->file_size;
    memcpy(dirBuffer, parentDE, sizeof(directory_entry));
    if(strcmp(pathCopy, "/")==0){
      memcpy( parentDE, dirBuffer+ sizeof(directory_entry), sizeof(directory_entry));
      parentDE->file_size = fdD->directory->file_size;
      memcpy(dirBuffer+ sizeof(directory_entry), parentDE, sizeof(directory_entry));
    }

    // printf("Adding file to Dir -> Writing 1 block to %d\n", fdD->directory->block_location);
    LBAwrite(dirBuffer, 1, fdD->directory->block_location);

    free(dirBuffer);
    free(DE);
    free(pathCopy);
    return 0;
}

int findDELocation(const char *filename) {
    if (filename == NULL) {
        printf("Invalid filename provided\n");
        return -1;
    }

    char *lastSlash = strrchr(filename, '/');
    if (lastSlash == NULL || *(lastSlash + 1) == '\0') {
        printf("Invalid path format or empty file name\n");
        return -1;
    }

    char *fileName = lastSlash + 1; // The actual file name

    // Create a copy of the path to the directory
    char *dirPath = strndup(filename, lastSlash - filename);
    if (!dirPath) {
        printf("Memory allocation failed for dirPath\n");
        return -1;
    }

    // Handle the root directory case
    if (strlen(dirPath) == 0) {
        free(dirPath);
        dirPath = strdup("/");
    }

    // Open the directory
    fdDir *dir = fs_opendir(dirPath);
    free(dirPath);

    if (dir == NULL) {
        printf("Directory not found\n");
        return -1;
    }

    struct fs_diriteminfo *dirInfo;
    int DElocation = -1;
    int offset = 0;

    // Iterate through the directory entries
    while ((dirInfo = fs_readdir(dir)) != NULL) {
        if (strcmp(dirInfo->d_name, fileName) == 0) {
            DElocation = dir->directory->block_location * B_CHUNK_SIZE + offset;
            break;
        }
        offset += sizeof(directory_entry);
    }

    fs_closedir(dir);
    return DElocation;
}



// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, O_CREAT, or O_RDWR
b_io_fd b_open(char *filename, int flags) {
    char *pathName = malloc(100);;
    if (startup == 0) {
        b_init();  // Initialize our file system if not already done
    }

    b_io_fd returnFd = b_getFCB();  // Get a free File Control Block
    if (returnFd == -1) {
        printf("No free File Control Block available\n");
        return -1;
    }

    fcbArray[returnFd].buf = (char *)malloc(B_CHUNK_SIZE);
    if (fcbArray[returnFd].buf == NULL) {
        printf("Memory allocation for buffer failed\n");
        return -1;
    }

    fcbArray[returnFd].buflen = 0;
    fcbArray[returnFd].flag = flags;
    fcbArray[returnFd].index = 0;

    if (strchr(filename, '/') == NULL){
        pathName = fs_getcwd(pathName, strlen(filename));
        memcpy(pathName+strlen(pathName),filename,strlen(filename));
    }

    if ((flags & (O_WRONLY | O_CREAT)) == (O_WRONLY | O_CREAT)) {
        if (createFile(pathName) != 0) {
            free(fcbArray[returnFd].buf);
            return -1;
        }
    } 


    struct fs_diriteminfo *fs_dir = findFile(pathName);
    if (fs_dir == NULL) {
        printf("Path/file does not exist\n");
        free(fcbArray[returnFd].buf);
        return -1;
    }

    fcbArray[returnFd].location = fs_dir->block_location;
    fcbArray[returnFd].fileSize = fs_dir->file_size;
    fcbArray[returnFd].currentBlock = fs_dir->block_location;

    fcbArray[returnFd].DELocation = findDELocation(pathName);

    return returnFd;  // Return the file descriptor
}


int UseNextFreeBlock(int previousBlock) {
    VCB *vcb = g_vcb;

    if (firstFreeBlock == -1) {
        printf("No free blocks available\n");
        return -1;
    }

    freeSpaceList[firstFreeBlock].in_use = 1;
    freeSpaceList[firstFreeBlock].end_of_file = 1;
    int allocatedBlock = firstFreeBlock;
    firstFreeBlock = freeSpaceList[firstFreeBlock].next_lba_block;

    if (previousBlock != -1) {
        freeSpaceList[previousBlock].next_lba_block = allocatedBlock;
        freeSpaceList[previousBlock].end_of_file = 0;
    }

    // printf("Updating Free Space List -> Writing block %d to %d\n",vcb->FAT_length , vcb->FAT_start);
    LBAwrite(freeSpaceList, vcb->FAT_length, vcb->FAT_start);

    return allocatedBlock;
}


/*
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
*/
int b_write(b_io_fd fd, char *buffer, int count) {
    if (startup == 0) b_init(); // Initialize our system

    if ((fd < 0) || (fd >= MAXFCBS)) return (-1); // Invalid file descriptor

    VCB *vcb = g_vcb;
    char *fileBlockBuffer = malloc(vcb->block_size);
    if (!fileBlockBuffer) return -1; // Memory allocation failed

    int parBufferIndex = 0;
    int totalBytesWritten = 0;
    int bytesToWrite;

    // Remaining bytes in the current block
    int remainingBytesInBlock = (fcbArray[fd].fileSize < vcb->block_size) ? 
                                (vcb->block_size - fcbArray[fd].fileSize) : 
                                (vcb->block_size - (fcbArray[fd].fileSize % vcb->block_size));

    memset(fileBlockBuffer, 0, vcb->block_size);
    if (remainingBytesInBlock > 0) {
        LBAread(fileBlockBuffer, 1, fcbArray[fd].currentBlock);
        bytesToWrite = (remainingBytesInBlock < count) ? remainingBytesInBlock : count;
        memcpy(fileBlockBuffer, buffer, bytesToWrite);
        printf("Writing to file -> Writing 1 block to %d\n", fcbArray[fd].currentBlock);
        LBAwrite(fileBlockBuffer, 1, fcbArray[fd].currentBlock);
        fcbArray[fd].fileSize += bytesToWrite; // Update file size
        totalBytesWritten += bytesToWrite;
        parBufferIndex += bytesToWrite;
        count -= bytesToWrite;
    }

    while (count > 0) {
        memset(fileBlockBuffer, 0, vcb->block_size);
        fcbArray[fd].currentBlock = UseNextFreeBlock(fcbArray[fd].currentBlock);
        bytesToWrite = (count < vcb->block_size) ? count : vcb->block_size;
        memcpy(fileBlockBuffer, buffer + parBufferIndex, bytesToWrite);
        printf("Writing to file -> Writing 1 block to %d\n", fcbArray[fd].currentBlock);
        LBAwrite(fileBlockBuffer, 1, fcbArray[fd].currentBlock);
        fcbArray[fd].fileSize += bytesToWrite; // Update file size
        totalBytesWritten += bytesToWrite;
        parBufferIndex += bytesToWrite;
        count -= bytesToWrite;
    }

    // Update the directory entry with the new file size
    directory_entry updatedDE;
    char *dirBuffer = malloc(vcb->block_size);
    if (dirBuffer == NULL) {
        free(fileBlockBuffer);
        return -1;
    }
    LBAread(dirBuffer, 1, fcbArray[fd].DELocation / vcb->block_size);
    memcpy(&updatedDE, dirBuffer + (fcbArray[fd].DELocation % vcb->block_size), sizeof(directory_entry));
    updatedDE.file_size = fcbArray[fd].fileSize;
    memcpy(dirBuffer + (fcbArray[fd].DELocation % vcb->block_size), &updatedDE, sizeof(directory_entry));
    LBAwrite(dirBuffer, 1, fcbArray[fd].DELocation / vcb->block_size);

    free(dirBuffer);
    free(fileBlockBuffer); // Free allocated memory
    return totalBytesWritten;
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

    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1); // Invalid file descriptor
    }

    b_fcb *fcb = &fcbArray[fd];
    if (fcb->flag == O_WRONLY) {
        printf("Cannot read file, flag is set to O_WRONLY\n");
        return (-1);
    }

    VCB *vcb = g_vcb;
    int bytesRead = 0;
    int part1 = 0, part2 = 0, part3 = 0;
    int remainingBytesInBuffer = fcb->buflen - fcb->index;
    int amountAlreadyDelivered = fcb->currentBlock * B_CHUNK_SIZE - remainingBytesInBuffer;

    if ((amountAlreadyDelivered + count) > fcb->fileSize) {
        count = fcb->fileSize - amountAlreadyDelivered;
    }

    if (remainingBytesInBuffer >= count) {
        part1 = count;
    } else {
        part1 = remainingBytesInBuffer;
    }

    if (part1 > 0) {
        memcpy(buffer, fcb->buf + fcb->index, part1);
        fcb->index += part1;
    }

    int numberOfBlocksToCopy = (count - remainingBytesInBuffer) / B_CHUNK_SIZE;
    part2 = numberOfBlocksToCopy * B_CHUNK_SIZE;
    part3 = count - remainingBytesInBuffer - part2;

    for (int i = 0; i < numberOfBlocksToCopy; i++) {
        bytesRead = LBAread(buffer + part1 + (i * B_CHUNK_SIZE), 1, fcb->currentBlock);
        if (bytesRead < B_CHUNK_SIZE) {
            part2 = bytesRead; // Adjust part2 if fewer bytes were read
            break;
        }

        // Update current block to the next block
        int FAT_BLOCK_location = vcb->FAT_start + (fcb->currentBlock / sizeof(FAT_block));
        int FAT_BLOCK_index = fcb->currentBlock % sizeof(FAT_block);
        FAT_block fatBlock;
        LBAread(&fatBlock, 1, FAT_BLOCK_location);
        fcb->currentBlock = fatBlock.next_lba_block;
    }

    if (part3 > 0) {
        bytesRead = LBAread(fcb->buf, 1, fcb->currentBlock);
        if (bytesRead < part3) {
            part3 = bytesRead; // Adjust part3 if fewer bytes were read
        }
        memcpy(buffer + part1 + part2, fcb->buf, part3);
        fcb->buflen = bytesRead; // Update buffer length
        fcb->index = part3;       // Set index for next read
    }

    return part1 + part2 + part3;
}


// Interface to Close the file
int b_close(b_io_fd fd) {}
