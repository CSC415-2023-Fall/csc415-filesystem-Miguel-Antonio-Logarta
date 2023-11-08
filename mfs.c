#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "fsLow.h"
#include "mfs.h"

void printVCB(VCB* vcb) {
	printf("Printing VCB\nmagic_signature: %ld\nvolume_size: %ld\nblock_size: %ld\nnum_blocks: %ld\nFAT_start: %ld\nFAT_length: %ld\nDE_start: %ld\nDE_length: %ld\n----------------------------------", 
  vcb->magic_signature,
  vcb->volume_size,
  vcb->block_size,
  vcb->num_blocks,
  vcb->FAT_start,
  vcb->FAT_length,
  vcb->DE_start,
  vcb->DE_length);
}


// Store out current working directory in memory
fdDir* g_fs_cwd = NULL;

int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Test for multiple inputs and conditions
// relative/path/to/file.txt
// ./dot/notation/to/indicate/current/path.c
// ../double/dot/to/go/up/to/parents
// /root/slash/to/indicate/absolute path
// /root/slash/../slash/to
// /root/slash/./to
fdDir * fs_opendir(const char *pathname) {
	/*
		We receive an absolute path from pathname, perform a DFS search to find the directory,
		then we return a pointer to that directory
	*/

  // TODO: Move "checking if pathname is valid" in a different function
  // If pathname starts with "/", absolutePath = true
  // Else, it is a relative path.

  if (pathname == NULL) {
    printf("Pathname is NULL\n");
    return NULL;
  }

  if (strlen(pathname) + 1 > MAX_PATH) {
    printf("Pathname exceeds MAX_PATH\n");
    return NULL;
  }

  // Variables for path parsing
  unsigned int absolutePath;
  char* path;
  char* token;
  char* lastToken;

  // Variables to read file directories
  VCB* fsVCB;
  unsigned char* buffer;
  int blocksRead;
  directory_entry* currDir;

  // Copy pathname
  path = (char*)malloc(MAX_PATH*sizeof(char));
  strncpy(path, pathname, MAX_PATH);

  // Check if pathname is absolute or relative path
  if (path[0] == '/') {
    absolutePath = 1;
  } else {
    absolutePath = 0;
  }

  // Read VCB and root directory
  fsVCB = getVCB();
  if (fsVCB == NULL) {
    printf("There was an error allocating the VCB\n");
    return NULL;
  }

  if (absolutePath == 1) {
    // Absolute directory, set starting dir to root directory
    // TODO: Load entire root directory to buffer using b_read
    buffer = malloc(fsVCB->DE_length*fsVCB->block_size);
    blocksRead = LBAread(buffer, fsVCB->DE_length, fsVCB->DE_start);
    memcpy(currDir, buffer, sizeof(directory_entry));
  } else {
    // Relative directory, set starting dir to current working directory
    if (g_fs_cwd == NULL) {
      printf("Current working directory has not been initialized yet!\n");
      return NULL;
    }

    buffer = malloc(g_fs_cwd->directory->file_size);
    // TODO: Check if LBAread can handle a buffer that is smaller than 512 bytes
    blocksRead = LBAread(buffer, fsVCB->DE_length, fsVCB->DE_start);
    memcpy(currDir, buffer, sizeof(directory_entry));
  }

  token = strtok(path, "/");
  while (token != NULL) {
    printf("Current folder: %s\n", currDir->name);
    lastToken = token;

    // Read each directory entry inside the current directory
    int subFolderFound = 0;
    for (int i = sizeof(directory_entry); i < currDir->file_size; i += sizeof(directory_entry)) {
      directory_entry* subfolder = (directory_entry*)malloc(sizeof(directory_entry));
      memcpy(subfolder, buffer + i, sizeof(directory_entry));

      if (subfolder->is_directory == 0) {
        if (strcmp(subfolder->name, token) == 0) {
          // Match found, set current folder to subfolder
          buffer = realloc(buffer, subfolder->file_size);
          blocksRead = LBAread(buffer, 1, subfolder->block_location);
          memcpy(currDir, buffer, sizeof(directory_entry));
          subFolderFound = 1;
        }
      }
      free(subfolder);
    }

    if (subFolderFound == 0) {
      // If no match has been found, return NULL since there is no subfolder to enter
      return NULL; 
    }

    token = strtok(NULL, "/");
  }

  printf("Last folder: %s\n", lastToken);

  // We reached the end of the path, return current directory
  fdDir* newDir = (fdDir*)malloc(sizeof(fdDir));
  // newDir->di = fs_readdir(newDir);
  // newDir.

  /*
    If absolute path, set current folder to root
    else, set current folder to cwd

    For each token
      Loop through each subfolder of the current folder
      If subfolder matches token
        set current folder to subfolder
        break
      else
        go to next loop
    
    After the for loop
    If the last token did not match the subfolder, there were no matches
      return NULL for we did not find any matching subfolder
    Else
      return last subfolder
  */

  // buffer = malloc(fsVCB->DE_length*fsVCB->block_size);
  // if (buffer == NULL) {
  //   printf("Couldn't allocate buffer\n");
  //   return NULL;
  // }

  // blocksRead = LBAread(buffer, fsVCB->DE_length, fsVCB->DE_start);
  // if (blocksRead < fsVCB->DE_length) {
  //   printf("There is an error getting the Root directory\n");
  //   return NULL;
  // }

  // currDir = (directory_entry*)malloc(sizeof(directory_entry));
  // if (currDir == NULL) {
  //   printf("Couldn't malloc memory for currDir\n");
  //   return NULL;
  // }

  // if (absolutePath == 1) {
  //   // Absolute directory, set starting dir to root directory
  //   memcpy(currDir, buffer, sizeof(directory_entry));
  // } else {
  //   // Relative directory, set starting dir to current working directory
  //   if (g_fs_cwd == NULL) {
  //     printf("Current working directory has not been initialized yet!\n");
  //     return NULL;
  //   }
  //   memcpy(currDir, g_fs_cwd->directory, sizeof(directory_entry));
  // }

  // Perform a DFS search to find directory

  // DFS handles from root only
  // Algorithm
  // Current directory
  // Copy contents of current directory to buffer
  // Go through child nodes by iterating through this buffer
  /*
    For each child node:
      if child node's name matches current token  
        if child node is a folder
          set current directory to child
        else
          return NULL since it is a file
      go to next child

    If for loop finishes, and no matching children were found, return NULL since no folders were found

  When tokens are empty, check the last current directory
  If the last current directory matches last token, return last current directory
  */ 
   
  // token = strtok(path, "/");
  // while (token != NULL) {
  //   printf("Current Folder: %s\n", token);

  //   // Load contents of directory into buffer
  //   realloc(buffer, currDir->file_size*sizeof(unsigned char));
  //   if (buffer == NULL) {
  //     printf("Couldn't reallocate buffer\n");
  //     return NULL;
  //   }
    
  //   // TODO: This is going to have to be replaced by b_read, because this can only handle one block instead of multiple
  //   blocksRead = LBAread(buffer, 1, currDir->block_location);
  //   if (blocksRead < fsVCB->DE_length) {
  //     printf("There is an error getting the Root directory\n");
  //     return NULL;
  //   }

  //   // Loop through the children of the buffer
  //   // Search children of current directory
  //   for (int i = 0; i < currDir->file_size; i += sizeof(directory_entry)) {
  //     directory_entry* currDir = (directory_entry*)malloc(sizeof(directory_entry));
  //     if (currDir == NULL) {
  //       printf("Couldn't malloc memory for current directory");
  //       return NULL;
  //     }

  //     memcpy(currDir, startDir + i, sizeof(directory_entry));

  //     if (strcmp(currDir->name, token) == 0) {
  //       if (currDir->is_directory == 0) {
  //         printf("Pathname contains a filename instead of a valid directory\n");
  //         // TODO: Make sure to free everything before exiting this function
  //         return NULL;
  //       }

  //       // Enter subfolder
  //       printf("Directory found! Ptr address: %ld\n", currDir->block_location);
  //       startDir = currDir;
  //     }

  //     free(currDir);
  //     printf("Current subfolder/file: %s", currDir->name);
  //   }


  //   for (int i = 0; i < startDir->file_size; i += sizeof(directory_entry)) {
  //   //   directory_entry* currDir = (directory_entry*)malloc(sizeof(directory_entry));
  //   //   memcpy(currDir, buffer + i, sizeof(directory_entry));

  //   //   printf("currdir: %s\n", currDir->name);

  //   //   if (strcmp(currDir->name, pathname) == 0) {
  //   //     printf("Directory found! Ptr address: %ld\n", currDir->block_location);
  //   //   }

  //   //   free(currDir);
  //   // }

  //   token = strtok(NULL, "/");
  // }

  // free(path);
  return NULL;

}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
  return NULL;
}

int fs_closedir(fdDir *dirp);

char *fs_getcwd(char *pathname, size_t size) {
  /*
    The getcwd() function copies an absolute path name of the current working
    directory to the array pointed to by buf, which is of length size.This
    function returns a null-terminated string containing an absolute path name
    that is the current working directory of the calling process. The path name
    is returned as the function result and via the argument buf, if present. If
    the length of the absolute path name of the current working directory,
    including the terminating null byte, exceeds size bytes, NULL is returned,
    and errno is set to ERANGE; an application should check for this error, and
    allocate a larger buffer if necessary.
  */

  // But how do we know the current working directory?
  // We have to know this from some state variable
}

int fs_setcwd(char *pathname) {
	// This is the equivalent of the linux chdir or cd
	// We receive an absolute pathname 
  // g_fs_cwd = fs_opendir(pathname);
  // if (!g_fs_cwd) {
  //   return -1;  // Failure to set cwd. Not a valid file path
  // } else {
  //   return 0;
  // }
  fs_opendir(pathname);
  return 0;
}

int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file

int fs_stat(const char *path, struct fs_stat *buf);

int writeTestFiles() {
  int blocksRead;
  int blocksWritten;
  unsigned char* buffer;
  char* strBuff;
  FAT_block* FATTable;

  // Read vcb
  VCB* vcb = getVCB();

  // Read FAT Table
  FATTable = (FAT_block*)malloc(vcb->block_size*vcb->FAT_length);
  if (FATTable == NULL) {
    perror("Error allocating file table");
    exit(EXIT_FAILURE);
  }

  buffer = (unsigned char*)malloc(vcb->block_size*vcb->FAT_length);
  blocksRead = LBAread(buffer, vcb->FAT_length, vcb->FAT_start);
  if (blocksRead < vcb->FAT_length) {
    perror("There was an error reading fat table");
    exit(EXIT_FAILURE);
  }
  memcpy(FATTable, buffer, vcb->block_size*vcb->FAT_length);

  // Reserve lba blocks 0 to 8
  for (int i = 1; i < 9; i++) {
    FATTable[i].in_use = 1;
    FATTable[i].end_of_file = 1;
  }

  // We have one file that spans two blocks
  FATTable[7].end_of_file = 0;
  memcpy(buffer, FATTable, vcb->block_size*vcb->FAT_length);
  blocksWritten = LBAwrite(buffer, vcb->FAT_length, vcb->FAT_start);
  if (blocksWritten < vcb->FAT_length) {
    perror("There was an error writing FAT table");
    exit(EXIT_FAILURE);
  }

  /*
    Now insert the directories into the file tree
  */
  directory_entry* dirBuffer = (directory_entry*)malloc(vcb->block_size);
  directory_entry* rootDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* parentDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* homeDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* desktopDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* notesDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* configDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* miscDir = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* someFile = (directory_entry*)malloc(sizeof(directory_entry));
  directory_entry* bigFile = (directory_entry*)malloc(sizeof(directory_entry));

  // Read root dir
  buffer = realloc(buffer, vcb->block_size*vcb->DE_length);
  if (buffer == NULL) {
    perror("realloc failed");
    exit(EXIT_FAILURE);
  }

  blocksRead = LBAread(buffer, vcb->DE_length, vcb->DE_start);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  memcpy(rootDir, buffer, sizeof(directory_entry));

  // In root parent folder, 
  // Create ..
  // Create Home directory
  // Create Desk directory
  // Create Notes directory
  // strncpy(rootDir->name, "rooooot", sizeof(rootDir->name) / sizeof(rootDir->name[0]));
  rootDir->file_size = sizeof(directory_entry)*5; // Number of subdirectories inside + itself + parent

  parentDir->block_location = vcb->DE_start;
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));          
  parentDir->is_directory = 1;   
  parentDir->file_size = rootDir->file_size;    // We are inserting 3 directories into the directory 
  parentDir->date_created = rootDir->date_created; 
  parentDir->last_modified = rootDir->last_modified; 

  homeDir->block_location = 155;
  strncpy(homeDir->name, "Home", sizeof(homeDir->name) / sizeof(homeDir->name[0]));          
  homeDir->is_directory = 1;   
  homeDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 subdirectories into the directory 
  homeDir->date_created = time(0); 
  homeDir->last_modified = time(0); 

  desktopDir->block_location = 156;
  strncpy(desktopDir->name, "Desktop", sizeof(desktopDir->name) / sizeof(desktopDir->name[0]));          
  desktopDir->is_directory = 1;   
  desktopDir->file_size = sizeof(directory_entry)*2; // Directory entry contains itself and its parent 
  desktopDir->date_created = time(0); 
  desktopDir->last_modified = time(0); 

  desktopDir->block_location = 157;
  strncpy(notesDir->name, "Notes", sizeof(notesDir->name) / sizeof(notesDir->name[0]));          
  notesDir->is_directory = 1;   
  notesDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 files into the directory 
  notesDir->date_created = time(0); 
  notesDir->last_modified = time(0); 

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], rootDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));
  memcpy(&dirBuffer[2], homeDir, sizeof(directory_entry));
  memcpy(&dirBuffer[3], desktopDir, sizeof(directory_entry));
  memcpy(&dirBuffer[4], notesDir, sizeof(directory_entry));

  blocksRead = LBAwrite(dirBuffer, 1, vcb->DE_start);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // Under home folder
  // Create parent directory
  // Create config directory
  // Create misc directory
  parentDir->block_location = rootDir->block_location;
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));          
  parentDir->is_directory = 1;   
  parentDir->file_size = homeDir->file_size;    // We are inserting 3 directories into the directory 
  parentDir->date_created = homeDir->date_created; 
  parentDir->last_modified = homeDir->last_modified; 

  configDir->block_location = 158;
  strncpy(configDir->name, "Config", sizeof(configDir->name) / sizeof(configDir->name[0]));          
  configDir->is_directory = 1;   
  configDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 subdirectories into the directory 
  configDir->date_created = time(0); 
  configDir->last_modified = time(0); 

  miscDir->block_location = 159;
  strncpy(miscDir->name, "Misc", sizeof(miscDir->name) / sizeof(miscDir->name[0]));          
  miscDir->is_directory = 1;   
  miscDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 subdirectories into the directory 
  miscDir->date_created = time(0); 
  miscDir->last_modified = time(0);

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], homeDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));
  memcpy(&dirBuffer[2], configDir, sizeof(directory_entry));
  memcpy(&dirBuffer[3], miscDir, sizeof(directory_entry));

  blocksRead = LBAwrite(dirBuffer, 1, homeDir->block_location);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // Create config directory entry
  memcpy(parentDir, homeDir, sizeof(directory_entry));
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], configDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));

  blocksRead = LBAwrite(dirBuffer, 1, configDir->block_location);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // Create misc directory entry
  memcpy(parentDir, homeDir, sizeof(directory_entry));
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], miscDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));

  blocksRead = LBAwrite(dirBuffer, 1, miscDir->block_location);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // Create the desktop directory entry
  memcpy(parentDir, rootDir, sizeof(directory_entry));
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], desktopDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));

  blocksRead = LBAwrite(dirBuffer, 1, desktopDir->block_location);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // Under the Notes directory
  // Create sometxt.txt file
  // Create Bigfile.txt file
  memcpy(parentDir, rootDir, sizeof(directory_entry));
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));

  notesDir->block_location = 157;
  strncpy(notesDir->name, "Notes", sizeof(notesDir->name) / sizeof(notesDir->name[0]));          
  notesDir->is_directory = 1;   
  notesDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 files into the directory 
  notesDir->date_created = rootDir->date_created; 
  notesDir->last_modified = rootDir->last_modified; 

  someFile->block_location = 160;
  strncpy(someFile->name, "someFile.txt", sizeof(someFile->name) / sizeof(someFile->name[0]));          
  someFile->is_directory = 0;   
  someFile->file_size = rootDir->file_size;
  someFile->date_created = rootDir->date_created; 
  someFile->last_modified = rootDir->last_modified; 

  bigFile->block_location = 161;
  strncpy(bigFile->name, "bigFile.txt", sizeof(bigFile->name) / sizeof(bigFile->name[0]));          
  bigFile->is_directory = 0;   
  bigFile->file_size = rootDir->file_size;
  bigFile->date_created = rootDir->date_created; 
  bigFile->last_modified = rootDir->last_modified; 

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], notesDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));
  memcpy(&dirBuffer[2], someFile, sizeof(directory_entry));
  memcpy(&dirBuffer[3], bigFile, sizeof(directory_entry));

  blocksWritten = LBAwrite(dirBuffer, 1, desktopDir->block_location);
  if (blocksWritten < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // Write somefile directory entry and insert data
  strBuff = (char*)malloc(100*sizeof(char));
  if (strBuff == NULL) {
    perror("Error allocating strbuf");
    exit(EXIT_FAILURE);
  }

  strncpy(strBuff, "This is someFile.txt that has a little bit of data inside of it", 100);

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], someFile, sizeof(directory_entry));
  memcpy(&dirBuffer[1], strBuff, 100);

  blocksWritten = LBAwrite(dirBuffer, 1, someFile->block_location);
  if (blocksWritten < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }


  printf("Writing Big File that spans 2 blocks\n");
  printf("Writing to block %ld\n", bigFile->block_location);
  // Write bigFile directory entry and insert data
  const char* bigString = "Life is a journey filled with twists and turns. It's a continuous adventure where we learn, grow, and experience the beauty of the world. Every day presents new opportunities and challenges, and it's up to us to make the most of them. Embrace the unknown, cherish the moments, and strive to be the best version of yourself. In this journey, remember that kindness, empathy, and love are the guiding stars that illuminate the path. So, let's keep moving forward with an open heart and a curious mind, making the most of every step we take";
  
  dirBuffer = realloc(dirBuffer, 2*vcb->block_size);
  if (strBuff == NULL) {
    perror("Error allocating dirbuffer");
    exit(EXIT_FAILURE);
  }

  strBuff = realloc(strBuff, 700);
  if (strBuff == NULL) {
    perror("Error allocating strbuf");
    exit(EXIT_FAILURE);
  }

  memset(dirBuffer, '\0', 2*vcb->block_size);
  memcpy(&dirBuffer[0], bigFile, sizeof(directory_entry));

  strncpy(strBuff, bigString, 700);
  strBuff[700 - 1] = '\0';
  memcpy(&dirBuffer[1], strBuff, 700);

  blocksWritten = LBAwrite(dirBuffer, 2, bigFile->block_location);
  if (blocksWritten < 2) {
    printf("%d %ld\n", blocksWritten, bigFile->block_location);
    perror("Error writing to disk");
    exit(EXIT_FAILURE);
  }
  // // // memcpy(&dirBuffer[1], strBuff, 100);

  // // blocksRead = LBAwrite(dirBuffer, 1, bigFile->block_location);
  // // if (blocksRead < 1) {
  // //   perror("Error reading root");
  // //   exit(EXIT_FAILURE);
  // // }
  // unsigned char* bigBuff = malloc(vcb->block_size*2);
  // if (bigBuff == NULL) {
  //   perror("Error reallocating strbuf");
  //   exit(EXIT_FAILURE);
  // }
  // printf("big buff\n");


  // realloc(strBuff, 800*sizeof(char));
  // if (strBuff == NULL) {
  //   perror("Error reallocating strbuf");
  //   exit(EXIT_FAILURE);
  // }

  // printf("str buff\n");

  // strncpy(strBuff, "Life is a journey filled with twists and turns. It's a continuous adventure where we learn, grow, and experience the beauty of the world. Every day presents new opportunities and challenges, and it's up to us to make the most of them. Embrace the unknown, cherish the moments, and strive to be the best version of yourself. In this journey, remember that kindness, empathy, and love are the guiding stars that illuminate the path. So, let's keep moving forward with an open heart and a curious mind, making the most of every step we take", 400);

  // // realloc(dirBuffer, vcb->block_size*2);

  // printf("strncpy compl\n");

  // memset(bigBuff, '\0', vcb->block_size*2);
  // printf("Preparing memcpy\n");
  // memcpy(&bigBuff, bigFile, sizeof(directory_entry));
  // memcpy(&bigBuff + sizeof(directory_entry), strBuff, 400);
  // printf("Preparing done memcpy\n");
  // printf("Here: %ld", bigFile->block_location);
  
  // blocksWritten = LBAwrite(bigBuff, 1, bigFile->block_location);
  // if (blocksWritten < 1) {
  //   printf("%ld\n", blocksWritten);
  //   perror("Error reading root");
  //   exit(EXIT_FAILURE);
  // }

  // printf("Done blocks read");

  // free(vcb);

  free(vcb);
  free(buffer);
  free(strBuff);
  free(FATTable);
  free(dirBuffer);
  free(rootDir);
  free(parentDir);
  free(homeDir);
  free(desktopDir);
  free(notesDir);
  free(configDir);
  free(miscDir);
  free(someFile);
  free(bigFile);
}

uint64_t getMinimumBlocks(uint64_t bytes, uint64_t blockSize) {
  double dBlocksNeeded = bytes/blockSize;
  uint64_t blocksNeeded = ceil(dBlocksNeeded);
  return blocksNeeded;
}