/**************************************************************
* Class:  CSC-415
* Names: Mohammed Deeb, Miguel Antonio Logarta, Abdarrahman Ayyaz, Avitaj Singh Bhangoo
* Student IDs: 922875899, 923062683, 922859909, 921303783
* Github Name:
* Group Name: Byte Builders
* Project: Basic File System
*
* File: mfs.c
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
*	There are also some extra utility functions defined in mfs.h
* to help simplify memory allocation and pathname parsing
**************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fsLow.h"
#include "mfs.h"
#include "debug.h"
#include "partition.h"

// Store out current working directory in memory
fdDir *g_fs_cwd = NULL;

struct new_directory {
  // Data inserted to make a new directory
  directory_entry newDirectory;
  directory_entry parent;
};

char* fs_getAbsolutePath(const char* path) {
  char* absolutePath = fs_malloc(MAX_PATH, "Unable to malloc absolute path");

  if (path[0] == '/') {
    // This is an absolute path, copy the whole path
    strncpy(absolutePath, path, MAX_PATH);
    concatStrings(absolutePath, "/", MAX_PATH);
  } else {
    // This is a relative path

    // Get cwd and append path
    absolutePath = malloc(MAX_PATH);
    absolutePath = fs_getcwd(absolutePath, MAX_PATH);

    concatStrings(absolutePath, "/", MAX_PATH);
    concatStrings(absolutePath, path, MAX_PATH);
  }

  return absolutePath;
}

int fs_mkdir(const char *pathname, mode_t mode) {
  /* Creates a directory. Returns 0 if sucecssful 
      TODO: Check file permissions with mode param
  */

  // Check if directory already exists
  // Search for free block in FAT
  // mark in_use = 1
    // If the file is bigger than 512 bytes, we're going to need to put this into a loop
  // Create directory entry
  // Write directory entry into thefree block
  // Read the parent directory
  // Insert pointer to new directory in parent directory
  // Write the parent directory
  // Write the new directory

  // Part 1: Check if path is valid, and check if directory already exists
  char* newDirPath = fs_getAbsolutePath(pathname);
  char* parentDirPath = fs_getAbsolutePath(pathname);
  char* newDirName = fs_malloc(MAX_PATH, "Unable to malloc new directory name");
  struct fs_stat newDirStat;
  struct fs_stat parentDirStat;
  int status;

  newDirPath = fs_formatPathname(newDirPath, ".");
  parentDirPath = fs_formatPathname(parentDirPath, "..");
  debug_print("Formatted Paths: %s %s, %d\n", newDirPath, parentDirPath, strlen(parentDirPath));

  // Check if parent exists
  status = fs_stat(parentDirPath, &parentDirStat);
  if (status != 0) {
    printf("Parent directory does not exist\n");
    free(newDirPath);
    free(parentDirPath);
    return -1;
  }

  // Check if directory we want to make already exists
  status = fs_stat(newDirPath, &newDirStat);
  if (status != 0) {
    debug_print("Directory does not exist! Good to go\n");
  } else {
    printf("Directory already exists!\n");
    free(newDirPath);
    free(parentDirPath);
    return -1;
  }

  // Part 2: Find space on the FAT
  FAT_block* FAT;
  fat_index freeBlock;
  lba_offset lbaOffset;
  int numBlocks;

  // Find free block in FAT table
  FAT = fs_getFAT();
  freeBlock = fs_findFreeBlock(FAT);
  numBlocks = fs_getFATLength();
  if (freeBlock < 0) {
    printf("No available remaining space!\n");
    free(newDirPath);
    free(parentDirPath);
    // fs_closedir(dirExists);
    fs_freefat(FAT);
    return -1;
  }

  // Reserve the block for writing
  FAT[freeBlock].in_use = 1;
  FAT[freeBlock].end_of_file = 1;

  debug_print("before writing %ld\n", freeBlock);

  fs_writeFAT(FAT, numBlocks);

  debug_print("after writing\n");

  lbaOffset = fs_getLBABlock(freeBlock);

  // Part 3: Create the directory entry and store to disk
  // Create new directory
  directory_entry* newDirectory = fs_malloc(sizeof(directory_entry), "Unable to malloc new directory");
  newDirectory->block_location = lbaOffset;
  newDirectory->date_created = time(0);
  newDirectory->file_size = sizeof(directory_entry)*2;
  newDirectory->is_directory = 1;
  newDirectory->last_modified = time(0);
  strncpy(newDirectory->name, newDirName, MAX_PATH);

  // Get address to parent directory
  fdDir* fdParentDir = fs_opendir(parentDirPath);
  directory_entry* parentDirectory = fs_malloc(sizeof(directory_entry), "Unable to malloc parent directory");
  memcpy(parentDirectory, fdParentDir->directory, sizeof(directory_entry));
  parentDirectory->file_size += sizeof(directory_entry);
  strncpy(parentDirectory->name, "..", MAX_PATH);

  // Pack directories together and store data into the disk
  struct new_directory data;
  memcpy(&data.newDirectory, newDirectory, sizeof(directory_entry));
  memcpy(&data.parent, parentDirectory, sizeof(directory_entry));

  fs_LBAwrite(&data, 1, lbaOffset, "Unable to write new directory to disk");

  // Part 4: Modify the parent directory to include pointer to the new directory
  unsigned char* buffer = fs_malloc_buff(parentDirStat.st_size + sizeof(directory_entry), parentDirStat.st_blksize, "Unable to read parent directory");
  fs_LBAread(buffer, 1, parentDirStat.st_block_location, "Unable to read parent dir from disk");
  memcpy(buffer + parentDirStat.st_size + 1, newDirectory, sizeof(directory_entry));
  memcpy(buffer, parentDirectory, sizeof(directory_entry));
  fs_LBAwrite(buffer, 1, parentDirStat.st_block_location, "Unable to write to disk!");

  free(newDirPath);
  free(parentDirPath);
  free(newDirName);
  fs_freefat(FAT);
  free(buffer);

  return 0;
}

int fs_rmdir(const char *pathname) {
  /* 
    When removing the target directory from the parent directory, 
    you can use memmove to shift all the directories after the target directory.
    Imagine that dircontents is an array of directory entries, you can use
    memmove to shift all the elements down after you delete an element. This
    takes one line of code and is way more efficient 
  */

  // Check if directory exists
  struct fs_stat dirStat;
  int status = fs_stat(pathname, &dirStat);

  if (status != 0) {
    printf("Directory doesn't exist!\n");
    return -1;
  }

  // Get pointer to parent dir

  
  // If directory exists check if there are subdirectories or files

  // Get starting FAT index from lba block number
  // while eof != true
    // set current index in_use=0
    // Go to next address
  
  // Read parent directory
  // Remove directory from parent
  // Write modified parent directory back to disk
  
  return -1;
}

char* fs_getLastToken(const char* path) {
  // Recieves a pathname and returns the last folder/file of that path
  char* destinationPath;
  char* filename;
  int strPos;
  int filenameLength;

  // Get the name of the file/directory. It is the last token
  destinationPath = fs_formatPathname(path, ".");
  filename = fs_malloc(MAX_PATH, "Unable to malloc filename");
  strPos = strlen(destinationPath);
  filenameLength = 0;

  // Start from the end of the string until we encounter the first slash
  while (destinationPath[strPos] != '/' && strPos >= 0) {
    filenameLength++;
    strPos--;
  }

  // Copy the characters after the slash
  // TODO: Bug test this
  memset(filename, '\0', MAX_PATH);
  strncpy(filename, destinationPath + strPos + 1, filenameLength);

  free(destinationPath);

  return filename; 
}

fdDir *fs_createFdDir(directory_entry *de, const char *absolutePath, char *dirContent) {
  // debug_print("absolute path recieved: %s\n", absolutePath);

  fdDir *returnFdDir = fs_malloc(sizeof(fdDir), "Unable to malloc returnFdDir");
  returnFdDir->directory = fs_malloc(sizeof(directory_entry), "Unable to malloc returnFdDir->directory");
  
  // returnFdDir->d_reclen = de->file_size;
  returnFdDir->d_reclen = sizeof(fdDir);
  returnFdDir->dirEntryPosition = 0;
  memcpy(returnFdDir->directory, de, sizeof(directory_entry));
  returnFdDir->di = NULL;

  strncpy(returnFdDir->absolutePath, absolutePath, MAX_PATH);

  // This should be a memcpy
  returnFdDir->dirContent = fs_malloc(de->file_size, "Unable to malloc dirContent");
  memcpy(returnFdDir->dirContent, dirContent, de->file_size);
  // returnFdDir->dirContent = dirContent;

  return returnFdDir;
}

fdDir* fs_opendir(const char *pathname) {
  /*
    Returns a file directory given the pathname.

    Notes: 
    More improved version of the old fs_opendir that can now properly handle . and ..
    TODO: Since b_read() has not been done yet, all directories can only support up to 
    512/64 = 8 directories, including itself.

    TODO (Design choice? Debating on it): 
      - fs_opendir should be able to return both files and directories, but we should give t an argument 
        to allow to happen
        For example:
          fs_opendir(pathname, FS_DIR) only returns directories
          fs_opendr(pathname, FS_DIR | FS_FILES) opens files and directories
  */
  // debug_print("Pathname: %s\n", pathname);

  if (pathname == NULL) {
    printf("Pathname is NULL\n");
    return NULL;
  }

  if (strlen(pathname) + 1 > MAX_PATH) {
    printf("Pathname exceeds MAX_PATH (%d characters)\n", MAX_PATH);
    return NULL;
  }

  // Variables for path parsing
  bool isAbsolutePath;
  char *path;
  char *absolutePath;
  char *token;

  // Variables to read file directories
  VCB *vcb;
  unsigned char *buffer;
  directory_entry *currentDirectory;
  unsigned char *directoryContents;
  uint64_t blocksRead;
  uint64_t blocksWritten;
  bool subfolderFound;
  fdDir* returnDir;

  // Check if pathname is absolute or relative path
  if (pathname[0] == '/') {
    isAbsolutePath = true;
  } else {
    isAbsolutePath = false;
    if (g_fs_cwd == NULL) {
      printf("Current working directory is not initialized yet!\n");
      return NULL;
    }
  }

  // Get VCB
  vcb = fs_getvcb();

  if (isAbsolutePath == true) {
    // debug_print("%s is an absolute path\n", pathname);

    // Load root directory as starting directory
    // TODO: This will be replaced by b_read()
    buffer = fs_malloc_buff(vcb->DE_length*vcb->block_size, vcb->block_size, "Error getting root");
    buffer = fs_LBAread(buffer, 1, vcb->DE_start, "Unable to read root");

    currentDirectory = fs_malloc(sizeof(directory_entry), "Error with malloc for current directory");
    memcpy(currentDirectory, buffer, sizeof(directory_entry));
    
    directoryContents = fs_malloc(currentDirectory->file_size, "Unable to malloc directory contents");
    memcpy(directoryContents, buffer, currentDirectory->file_size);

    path = fs_malloc(MAX_PATH, "Error allocating memory for path\n");
    strncpy(path, pathname, MAX_PATH);

    // When it an absolute path, just replace the path with the pathname
    absolutePath = fs_formatPathname("/", pathname);

    // debug_print("Absolute path: %s is initialized\n", absolutePath);
  } else {
    // debug_print("%s is a relative path\n", pathname);

    // // Load current working directory as starting directory
    // debug_print(
    //   "g_fs_cwd: reclen %d, lba: %ld\n",
    //   g_fs_cwd->d_reclen,
    //   g_fs_cwd->directory->block_location
    // );

    buffer = fs_malloc_buff(g_fs_cwd->d_reclen, vcb->block_size, "Error getting root");
    buffer = fs_LBAread(buffer, 1, g_fs_cwd->directory->block_location, "Unable to read root");

    currentDirectory = fs_malloc(sizeof(directory_entry), "Error with malloc for current directory");
    memcpy(currentDirectory, buffer, sizeof(directory_entry));
    
    directoryContents = fs_malloc(currentDirectory->file_size, "Unable to malloc directory contents");
    memcpy(directoryContents, buffer, currentDirectory->file_size);

    path = fs_malloc(MAX_PATH, "Error allocating memory for path\n");
    strncpy(path, pathname, MAX_PATH);

    // When the path is relative, append the relative path to the current working directory
    absolutePath = fs_formatPathname(g_fs_cwd->absolutePath, pathname);
    // debug_print("Relative path %s is initialized\n", absolutePath);
  }
  
  subfolderFound = true;
  token = strtok(path, "/");
  while (token != NULL) {
    // debug_print("current token: %s\n", token);
    
    subfolderFound = false;   // Reset flag

    // Skip "."" in path
    if (strcmp(token, ".") == 0) {
      subfolderFound = true;
    } else {
    // if (strcmp(token, ".") != 0) {
      // debug_print("%s\n", currentDirectory->name);

      // Loop through every subfolder of the current folder
      for (int i = sizeof(directory_entry); i < currentDirectory->file_size; i += sizeof(directory_entry)) {
        directory_entry* subfolder = fs_malloc(sizeof(directory_entry), "Unable to malloc subfolder");
        memcpy(subfolder, directoryContents + i, sizeof(directory_entry));

        // debug_print("|_%s\n", subfolder->name);

        if (subfolder->is_directory == 1 && strcmp(subfolder->name, token) == 0) {
          // debug_print("Subfolder found\n");
          subfolderFound = true;

          // Set current directory to this subfolder
          // Load current working directory as starting directory

          // TODO: Change to realloc
          free(buffer);
          buffer = fs_malloc_buff(subfolder->file_size, vcb->block_size, "Error getting subfolder");
          buffer = fs_LBAread(buffer, 1, subfolder->block_location, "Unable to read root");

          currentDirectory = realloc(currentDirectory, sizeof(directory_entry));
          if (currentDirectory == NULL) {
            printf("Unable to realloc current directory\n");
            exit(EXIT_FAILURE);
          }
          memcpy(currentDirectory, buffer, sizeof(directory_entry));

          directoryContents = realloc(directoryContents, currentDirectory->file_size);
          if (directoryContents == NULL) {
            printf("Unable to realloc directory contents\n");
            exit(EXIT_FAILURE);
          }
          memcpy(directoryContents, buffer, currentDirectory->file_size);
          
          free(subfolder);
          break;
        }

        free(subfolder);
      }

      if (subfolderFound == false) {
        break;
      }    
    }
    token = strtok(NULL, "/");
  }

  if (subfolderFound == true) {
    // debug_print("Path is valid, returning fd\n");
    // concatStrings(absolutePath, "/", MAX_PATH);
    // We have to format absolutePath first before passing it in
    returnDir = fs_createFdDir(currentDirectory, absolutePath, directoryContents);
  } else {
    // debug_print("Path is not valid, returning NULL\n");
    returnDir = NULL;
  }

  free(path);
  free(absolutePath);
  free(vcb);
  free(buffer);
  free(currentDirectory);
  free(directoryContents);

  return returnDir;
}


char* fs_formatPathname(const char* startPath, const char* destPath) {
  if (startPath != NULL && startPath[0] != '/') {
    printf("Start path has to be an absolute path!\n");
    return NULL;
  }

  // debug_print("---------------------------------\n");
  // debug_print("startPath: %s, destPath: %s\n", startPath, destPath);

  // Receives a pathname, and removes the . and .. in the pathname
  // Pathname has to be well formatted in advance
  int strPos;
  char* formattedPath = fs_malloc(MAX_PATH, "Failed to malloc formattedPath");

  char* startPathCopy = fs_malloc(MAX_PATH, "Failed to malloc startPathCopy");
  char* destPathCopy = fs_malloc(MAX_PATH, "Failed to malloc destPathCopy");
  int stackSize = 0;
  char* token;
  char** tokenStack = fs_malloc(sizeof(char*)*100, "Failed to malloc token stack");
  
  strncpy(startPathCopy, startPath, MAX_PATH);
  strncpy(destPathCopy, destPath, MAX_PATH);
  // debug_print("Done copying strings\n");

  // If it is a relative path, push tokens of the old path to the stack
  if (destPathCopy[0] != '/') {
    // debug_print("This is a relative path\n");
    token = strtok(startPathCopy, "/");
    while (token != NULL) {
      tokenStack[stackSize] = token;
      stackSize++;
      token = strtok(NULL, "/");
    }
  } else {
    // debug_print("This is an absolute path\n");
  }

  for (int i = 0; i < stackSize; i++) {
    // debug_print("%s\n", tokenStack[i]);
  }

  // Tokenize new path and remove the . and ..
  token = strtok(destPathCopy, "/");
  while (token != NULL) {
    // debug_print("curr token: %s\n", token);
    if (strcmp(token, ".") != 0) { // . means this current directory, so skip this one
      if (strcmp(token, "..") == 0) { // If .., go up a directory by popping the stack
        if (stackSize > 0) {
          tokenStack[stackSize - 1] = NULL;
          stackSize--;
        }
      } else {
        tokenStack[stackSize] = token;
        stackSize++;
      }
    }

    token = strtok(NULL, "/");
  }

  // debug_print("After new path tokenization\n");
  // for (int i = 0; i < stackSize; i++) {
  //   debug_print("%s\n", tokenStack[i]);
  // }

  // Combine tokens with a / between them to make the absolute path
  strPos = 0;
  memset(formattedPath, '\0', MAX_PATH);
  for (int i = 0; i < stackSize; i++) {
    // Prepend token with a slash
    formattedPath[strPos] = '/';
    strPos++;

    // Copy token to formattedPath
    strncpy(formattedPath + strPos, tokenStack[i], strlen(tokenStack[i]));
    strPos += strlen(tokenStack[i]);
  }

  // If our formatted path is empty, just return root
  if (strlen(formattedPath) == 0) {
    formattedPath[0] = '/';
  }

  free(tokenStack);
  free(startPathCopy);
  free(destPathCopy);

  // debug_print("formatted path: %s\n", formattedPath);
  return formattedPath;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
  // get directory from fdDir dirp
  // read contents of directory from the disk
  // Each time that readdir is called, we incremement pointer pointing to the contents

  /*
    BUGS:
      - When readdir is called after ls, there is some sort of misalignment caused.
      - The parent directory's name is referred to its actual name instead of ".."
      - 
  */

  // When it goes past, we return null
  // if (dirp->dirEntryPosition >= dirp->d_reclen) {
  if (dirp->dirEntryPosition >= dirp->directory->file_size) {
    // printf("Dir entry position greater than dirp: %d > %d\n", dirp->dirEntryPosition, dirp->d_reclen);
    dirp->dirEntryPosition = sizeof(directory_entry);
    // free(dirp->dirContent);
    // debug_print("Readdir pointer has exceeded it's limiit\n");
    return NULL;
  }

  if (dirp->dirContent == NULL) {
    // debug_print("dirp dircontent is null\n");

    // Read file from the disk
    VCB* fsVCB;
    char* buffer;
    fsVCB = fs_getvcb();
    buffer = fs_malloc_buff(dirp->directory->file_size, fsVCB->block_size, "Unable to malloc dirp buffer");
    
    uint64_t blocksToRead = getMinimumBlocks(dirp->directory->file_size, fsVCB->block_size);

    // TODO: Replace with b_read()
    fs_LBAread(buffer, blocksToRead, dirp->directory->block_location, "Unable to read file");
    dirp->dirContent = malloc(dirp->directory->file_size);
    memcpy(dirp->dirContent, buffer, dirp->directory->file_size);

    // Skip the first directory entry since the
    // frst directory entry is information about the directory itself
    dirp->dirEntryPosition = sizeof(directory_entry);

    free(fsVCB);
    free(buffer);
  }

  // Read next directory 
  // dirp->dirEntryPosition += sizeof(directory_entry);
  directory_entry *currDir = fs_malloc(sizeof(directory_entry), "Unable to malloc currDir");

  // debug_print("before: %d\n", dirp->dirEntryPosition);
  // debug_print("dirp->dirContent + dirp->dirEntryPosition: %p\n", &dirp->dirContent[0] + dirp->dirEntryPosition);
  // The bug occurs here
  memcpy(currDir, dirp->dirContent + dirp->dirEntryPosition, sizeof(directory_entry));
  dirp->dirEntryPosition += sizeof(directory_entry);
  // debug_print("after: %d\n", dirp->dirEntryPosition);

  // TODO: In the section below, we can just directly assign
  // to dirp->di instead of having to make DEInfo 

  // Return info through diriteminfo struct
  struct fs_diriteminfo *DEInfo = fs_malloc(sizeof(struct fs_diriteminfo), "Unable to malloc diriteminfo");
  DEInfo->d_reclen = sizeof(struct fs_diriteminfo);
  DEInfo->fileType = currDir->is_directory == 1 ? FT_DIRECTORY : FT_REGFILE;
  
  // Rename directory to "." If the current subdirectory is itself.
  if (strcmp(currDir->name, dirp->directory->name) == 0) {
    // BUG: "." is not being copied properly
    strncpy(DEInfo->d_name, ".", MAX_PATH);
  } else {
    strncpy(DEInfo->d_name, currDir->name, MAX_PATH);
  }

  DEInfo->block_location = currDir->block_location;
  DEInfo->file_size = currDir->file_size;
  DEInfo->date_created = currDir->date_created;
  DEInfo->last_modified = currDir->last_modified;

  // Free memory of di from previous call of readdir
  // TODO: We might need to check if dirp->di is null
  free(dirp->di);
  dirp->di = DEInfo;

  // dirp->di->
  // // printf("Dir entry position and dirp reclen: %d > %d\n", dirp->dirEntryPosition, dirp->d_reclen);
  // if (dirp->dirEntryPosition > dirp->d_reclen) {
  //   // printf("Dir entry position greater than dirp: %d > %d\n", dirp->dirEntryPosition, dirp->d_reclen);
  //   return NULL;
  // }

  free(currDir);

  return DEInfo;
}

int fs_closedir(fdDir *dirp) {
  /* Frees fdDir struct from memory */
  free(dirp->directory);
  free(dirp->di);
  free(dirp->dirContent);
  free(dirp);
  return 0;
}

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
  if (g_fs_cwd == NULL) {
    debug_print("g_fs_cwd is NULL!");
    return NULL;
  }

  // BUG: The realloc doesn't work

  // Pathname is basically our buffer to fill, we also return the pointer to this buffer
  // If NULL is passed to pathname, we allocate a new string buffer
  // debug_print("Size: %ld, strlen: %ld\n", size, strlen(g_fs_cwd->absolutePath) + 1);
  // if (pathname == NULL || size < strlen(g_fs_cwd->absolutePath) + 1) {
  //   int absolutePathSize = strlen(g_fs_cwd->absolutePath) + 1;
  //   pathname = realloc(pathname, absolutePathSize);
  //   if (pathname == NULL) {
  //     perror("Reallocation of pathname failed!");
  //     exit(EXIT_FAILURE);
  //   }
  //   strncpy(pathname, g_fs_cwd->absolutePath, absolutePathSize);

  //   // for (int i = 0; i < absolutePathSize; i++) {
  //   //   debug_print("%d, %c\n", i, pathname[i]);
  //   // }
  // } else {
  //   strncpy(pathname, g_fs_cwd->absolutePath, size);
  // }

  int absolutePathSize = strlen(g_fs_cwd->absolutePath) + 1;
  if (pathname == NULL) {
    // debug_print("pathname is NULL!");
    pathname = malloc(absolutePathSize);
    strncpy(pathname, g_fs_cwd->absolutePath, absolutePathSize);
  } else if (size < absolutePathSize) {
    pathname = realloc(pathname, absolutePathSize);
    if (pathname == NULL) {
      perror("Reallocation of pathname failed!");
      exit(EXIT_FAILURE);
    }
    strncpy(pathname, g_fs_cwd->absolutePath, absolutePathSize);
  } else {
    strncpy(pathname, g_fs_cwd->absolutePath, size);
  }
  
  return pathname;
}


int fs_setcwd(char *pathname) {
  // This is the equivalent of the linux chdir or cd
  fdDir* newCwd;
  newCwd = fs_opendir(pathname);
  
  if (newCwd == NULL) {
    printf("Error reallocating new g_fs_cwd\n");
    return -1;
  } else {
    // if (g_fs_cwd == NULL) {
    //   g_fs_cwd = fs_malloc(sizeof(fdDir), "Error reallocating g_fs_cwd!\n");
    // } else {
    //   fs_closedir(g_fs_cwd);
    // }

    if (g_fs_cwd != NULL) {
      fs_closedir(g_fs_cwd);
    }

    g_fs_cwd = newCwd;

    // memcpy(g_fs_cwd, newCwd, sizeof(fdDir));

    return 0;
  }
}

int fs_isFile(char *filename) {
  // Use fs_stat to get information about the file
  return fs_isDir(filename);
}

int fs_isDir(char *pathname) {
  // Use fs_stat to get information about the file
  struct fs_stat statbuf;
  int statStatus = fs_stat(pathname, &statbuf);
  if (statStatus != 0) {
    debug_print("Pathname is invalid\n");
    return -1;
  }

  return statbuf.st_filetype == FT_DIRECTORY;

  // return 0;
}

int fs_delete(char *filename) {
  // Removes a file

  // Get file information
  struct fs_stat fileStat;
  int status = fs_stat(filename, &fileStat);
  if (status != 0) {
    printf("File not found!\n");
    return -1;
  }

  // Part 1: Free every fat block used by the file
  FAT_block* fat = fs_getFAT();
  int numBlocks = fs_getFATLength();

  int currentBlock = fileStat.st_block_location;
  while (fat[currentBlock].end_of_file == 0) {
    fat[currentBlock].in_use = 0;
    currentBlock = fat[currentBlock].next_lba_block;
  }

  fat = fs_writeFAT(fat, numBlocks);
  fs_freefat(fat);

  // Part 2: Delete the file's directory entry from parent directory
}

int fs_stat(const char *path, struct fs_stat *buf) {
  /* 
    Fetches information about a file/directory via the fs_stat struct
    Returns -1 on failure
    Returns 0 on success
  */
  // debug_print("-----------fs_stat() called----------\n");
  // debug_print("Directory: %s\n", path);

  /*
    If it is an absolute path:
      Get the path without the last folder/file in front of the slash
      ex: /home/example/path/file -> /home/example/path
      ex: /home -> /
      ex: / -> /
    Else if it is a relative path:
      Get the cwd without the last folder/file in front of the slash
      ex: absolutePath=/home/example/path/file, relPath=file -> absolutePath=/home/example/path/
  */

  if (path == NULL) {
    return -1;    // Path not specified
  }

  char* startingPath;
  char* destPath;
  char* filename;
  char* parentFolder;
  int strPos;
  int filenameLength;
  fdDir* parentDir;
  VCB* vcb;

  if (path[0] == '/') {
    // This is an absolute path, 
    startingPath = fs_malloc(MAX_PATH, "Unable to malloc startingPath");
    strncpy(startingPath, path, MAX_PATH);
    concatStrings(startingPath, "/", MAX_PATH);
  } else {
    // This is a relative path

    // Get cwd and append it with path
    startingPath = fs_malloc(MAX_PATH, "Unabel to malloc startingPath");
    startingPath = fs_getcwd(startingPath, MAX_PATH);

    concatStrings(startingPath, "/", MAX_PATH);
    concatStrings(startingPath, path, MAX_PATH);
  }

  // Get the name of the file/directory. It is the last token
  destPath = fs_formatPathname(startingPath, ".");
  filename = fs_malloc(MAX_PATH, "Unable to malloc filename");
  strPos = strlen(destPath);
  filenameLength = 0;

  while (destPath[strPos] != '/' && strPos >= 0) {
    filenameLength++;
    strPos--;
  }
  memset(filename, '\0', MAX_PATH);

  // TODO: Bug test this
  strncpy(filename, destPath + strPos + 1, filenameLength);

  parentFolder = fs_formatPathname(startingPath, ".."); // Get absolute path of parent folder
  parentDir = fs_opendir(parentFolder);                // Open parent directory

  if (parentDir == NULL) {
    // debug_print("Parent dir returned null!\n");
    return -1;
  }

  struct fs_diriteminfo* di = fs_readdir(parentDir);          // Read contents of parent directory
  vcb = fs_getvcb();

  bool fileFound = false;

  while (di != NULL) {
    // debug_print("d_name: %s, filename: %s\n", di->d_name, filename);
    if (strcmp(di->d_name, filename) == 0) {
      // debug_print("Match found!\n");
      buf->st_size = di->file_size;
      buf->st_blksize = vcb->block_size;
      buf->st_blocks = fs_getMinimumBlocks(di->file_size, vcb->block_size);
      buf->st_accesstime = time(0);
      buf->st_modtime = di->last_modified;
      buf->st_createtime = di->date_created;
      buf->st_filetype = di->fileType;
      buf->st_block_location = di->block_location;
      fileFound = true;
      buf->st_location = di->block_location;
    }
    di = fs_readdir(parentDir);
  }

  fs_closedir(parentDir);
  fs_freevcb(vcb);

  free(startingPath);
  free(destPath);
  free(filename);
  free(parentFolder);

  if (fileFound == true) {
    return 0;
  } else {
    return -1;
  }
}

uint64_t getMinimumBlocks(uint64_t bytes, uint64_t blockSize) {
  // Returns amount of blocks of size blockSize greater than bytes 
  // debug_print("bytes: %ld, blocksize: %ld\n", bytes, blockSize);

  if (blockSize == 0) {
    perror("Division by zero error");
    exit(EXIT_FAILURE);
  }

  uint64_t blocksNeeded = bytes / blockSize;
  uint64_t remainder = bytes % blockSize;
  if (remainder > 0) {
    return blocksNeeded + 1;
  } else {
    return blocksNeeded;
  }
}

int writeTestFiles() {
  /*
    Writes sample directories and files to the file system
            root ("/")
            /   |     \
        home   desk  notes
       /  \           /           \
    config misc   someFile.txt    bigFile.txt

  */

  int blocksRead;
  int blocksWritten;
  unsigned char *buffer;
  char *strBuff;
  FAT_block *FATTable;

  // Read vcb
  debug_print("Getting the vcb for writeTestfiles");
  VCB *vcb = fs_getvcb();

  // Read FAT Table
  FATTable = malloc(vcb->block_size * vcb->FAT_length);
  if (FATTable == NULL) {
    perror("Error allocating file table");
    exit(EXIT_FAILURE);
  }

  buffer = malloc(vcb->block_size * vcb->FAT_length);
  if (buffer == NULL) {
    perror("Error allocating buffer");
    exit(EXIT_FAILURE);
  }
  
  blocksRead = LBAread(buffer, vcb->FAT_length, vcb->FAT_start);
  if (blocksRead < vcb->FAT_length) {
    perror("There was an error reading fat table");
    exit(EXIT_FAILURE);
  }
  memcpy(FATTable, buffer, vcb->block_size * vcb->FAT_length);

  // Reserve lba blocks 0 to 8
  for (int i = 1; i < 9; i++) {
    FATTable[i].in_use = 1;
    FATTable[i].end_of_file = 1;
  }

  // We have one file that spans two blocks
  FATTable[7].end_of_file = 0;
  memcpy(buffer, FATTable, vcb->block_size * vcb->FAT_length);
  blocksWritten = LBAwrite(buffer, vcb->FAT_length, vcb->FAT_start);
  if (blocksWritten < vcb->FAT_length) {
    perror("There was an error writing FAT table");
    exit(EXIT_FAILURE);
  }

  /*
    Now insert the directories into the file tree
  */
  directory_entry *dirBuffer = malloc(vcb->block_size);
  directory_entry *rootDir = malloc(sizeof(directory_entry));
  directory_entry *parentDir = malloc(sizeof(directory_entry));
  directory_entry *homeDir = malloc(sizeof(directory_entry));
  directory_entry *desktopDir = malloc(sizeof(directory_entry));
  directory_entry *notesDir = malloc(sizeof(directory_entry));
  directory_entry *configDir = malloc(sizeof(directory_entry));
  directory_entry *miscDir = malloc(sizeof(directory_entry));
  directory_entry *someFile = malloc(sizeof(directory_entry));
  directory_entry *bigFile = malloc(sizeof(directory_entry));

  // Read root dir
  buffer = realloc(buffer, vcb->block_size * vcb->DE_length);
  if (buffer == NULL) {
    perror("realloc failed");
    exit(EXIT_FAILURE);
  }

  memset(buffer, '\0', vcb->block_size * vcb->DE_length);

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
  // strncpy(rootDir->name, "rooooot", sizeof(rootDir->name) /
  // sizeof(rootDir->name[0]));
  rootDir->file_size = sizeof(directory_entry)*5; // Number of subdirectories inside + itself + parent

  parentDir->block_location = vcb->DE_start;
  strncpy(parentDir->name, "..", sizeof(parentDir->name) / sizeof(parentDir->name[0]));
  parentDir->is_directory = 1;
  parentDir->file_size = rootDir->file_size; // We are inserting 3 directories into the directory
  parentDir->date_created = rootDir->date_created;
  parentDir->last_modified = rootDir->last_modified;

  // debug_print("Start of lba blocks: %ld\n", vcb->DE_start);
  // debug_print("End of FAT: %ld\n", vcb->FAT_length + vcb->FAT_start - 1);

  homeDir->block_location = 155; 
  strncpy(homeDir->name, "Home", sizeof(homeDir->name) / sizeof(homeDir->name[0]));
  homeDir->is_directory = 1;
  homeDir->file_size = sizeof(directory_entry) * 4; // We are inserting 2 subdirectories into the directory
  homeDir->date_created = time(0);
  homeDir->last_modified = time(0);

  desktopDir->block_location = 156;
  strncpy(desktopDir->name, "Desktop", sizeof(desktopDir->name) / sizeof(desktopDir->name[0]));
  desktopDir->is_directory = 1;
  desktopDir->file_size = sizeof(directory_entry) * 2; // Directory entry contains itself and its parent
  desktopDir->date_created = time(0);
  desktopDir->last_modified = time(0);

  notesDir->block_location = 157;
  strncpy(notesDir->name, "Notes", sizeof(notesDir->name) / sizeof(notesDir->name[0]));
  notesDir->is_directory = 1;
  notesDir->file_size = sizeof(directory_entry) *4; // We are inserting 2 files into the directory
  notesDir->date_created = time(0);
  notesDir->last_modified = time(0);

  // printf("dirbuffer size: %d\n", 5 * sizeof(directory_entry));

  // Dir buffer is not being malloc'd properly
  // dirBuffer = malloc(5*sizeof(directory_entry));
  // dirBuffer = calloc(5, sizeof(directory_entry));
  // randomBuffer = calloc(5, sizeof(directory_entry));
  // if (randomBuffer == NULL) {
  //   printf("Error allocating randomBuffer for some reason\n");
  //   exit(EXIT_FAILURE);
  // }

  // printf("strncpy max size: %d\n", sizeof(notesDir->name) / sizeof(notesDir->name[0]));
  // memcpy(&randomBuffer[0], rootDir, sizeof(directory_entry));
  // memcpy(&randomBuffer[1], parentDir, sizeof(directory_entry));
  // memcpy(&randomBuffer[2], homeDir, sizeof(directory_entry));
  // memcpy(&randomBuffer[3], desktopDir, sizeof(directory_entry));
  // memcpy(&randomBuffer[4], notesDir, sizeof(directory_entry));

  // // printf("After dirbuffer the size is: %d\n", 5 * sizeof(directory_entry));

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], rootDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));
  memcpy(&dirBuffer[2], homeDir, sizeof(directory_entry));
  memcpy(&dirBuffer[3], desktopDir, sizeof(directory_entry));
  memcpy(&dirBuffer[4], notesDir, sizeof(directory_entry));

  /* TODO: Valgrind (memory checker) says:
      Syscall param write(buf) points to uninitialised byte(s)
      Address 0x4c7dd00 is 288 bytes inside a block of size 512 alloc'd
      Priority low.

      How weird, dirBuffer is supposed to fill up to 5*64 = 320 bytes
      There is a 32 byte difference and idk what's wrong
  */
  memset(buffer, '\0', vcb->block_size);
  memcpy(buffer, dirBuffer, 5*sizeof(directory_entry));
  blocksRead = LBAwrite(buffer, 1, vcb->DE_start);
  if (blocksRead < 1) {
    perror("Error reading root");
    exit(EXIT_FAILURE);
  }

  // randomBuffer = realloc(randomBuffer, vcb->block_size);
  // printf("Attemping realloc of dirBuffer\n");
  // dirBuffer = realloc(dirBuffer, vcb->block_size);

  // Under home folder
  // Create parent directory
  // Create config directory
  // Create misc directory
  parentDir->block_location = rootDir->block_location;
  strncpy(parentDir->name, "..", sizeof(parentDir->name) /
  sizeof(parentDir->name[0])); parentDir->is_directory = 1;
  parentDir->file_size = homeDir->file_size;    // We are inserting 3 directories into the directory 
  parentDir->date_created = homeDir->date_created; 
  parentDir->last_modified = homeDir->last_modified;

  configDir->block_location = 158;
  strncpy(configDir->name, "Config", sizeof(configDir->name) /
  sizeof(configDir->name[0])); configDir->is_directory = 1;
  configDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 subdirectories into the directory 
  configDir->date_created = time(0);
  configDir->last_modified = time(0);

  miscDir->block_location = 159;
  strncpy(miscDir->name, "Misc", sizeof(miscDir->name) /
  sizeof(miscDir->name[0])); miscDir->is_directory = 1; miscDir->file_size =
  sizeof(directory_entry)*4;    // We are inserting 2 subdirectories into the directory 
  miscDir->date_created = time(0); 
  miscDir->last_modified =time(0);

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
  strncpy(parentDir->name, "..", sizeof(parentDir->name) /
  sizeof(parentDir->name[0]));

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
  strncpy(parentDir->name, "..", sizeof(parentDir->name) /
  sizeof(parentDir->name[0]));

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
  strncpy(parentDir->name, "..", sizeof(parentDir->name) /
  sizeof(parentDir->name[0]));

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], desktopDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));

  // printf("Writing desktop %s %d\n", desktopDir->name, desktopDir->block_location);
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

  const char* someFileString = "This is someFile.txt that has a little bit of data inside of it. Blah Blah Blah lorem ipsum wawa.";
  const char* bigFileString = "Life is a journey filled with twists and turns. It's a continuous adventure where we learn, grow, and experience the beauty of the world. Every day presents new opportunities and challenges, and it's up to us to make the most of them. Embrace the unknown, cherish the moments, and strive to be the best version of yourself. In this journey, remember that kindness, empathy, and love are the guiding stars that illuminate the path. So, let's keep moving forward with an open heart and a curious mind, making the most of every step we take";

  notesDir->block_location = 157;
  strncpy(notesDir->name, "Notes", sizeof(notesDir->name) / sizeof(notesDir->name[0])); 
  notesDir->is_directory = 1; 
  notesDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 files into the directory 
  notesDir->date_created = rootDir->date_created;
  notesDir->last_modified = rootDir->last_modified;

  someFile->block_location = 160;
  strncpy(someFile->name, "someFile.txt", sizeof(someFile->name) / sizeof(someFile->name[0])); 
  someFile->is_directory = 0; 
  someFile->file_size = sizeof(directory_entry) + strlen(someFileString) + 1;
  // debug_print("Somefile size: %ld", sizeof(directory_entry) + strlen(someFileString) + 1);
  someFile->date_created = rootDir->date_created;
  someFile->last_modified = rootDir->last_modified;

  bigFile->block_location = 161;
  strncpy(bigFile->name, "bigFile.txt", sizeof(bigFile->name) / sizeof(bigFile->name[0])); 
  // debug_print("bigfile size: %ld", sizeof(directory_entry) + strlen(bigFileString) + 1);
  bigFile->is_directory = 0; 
  bigFile->file_size = sizeof(directory_entry) + strlen(bigFileString) + 1; 
  bigFile->date_created = rootDir->date_created;
  bigFile->last_modified = rootDir->last_modified;

  memset(dirBuffer, '\0', vcb->block_size);
  memcpy(&dirBuffer[0], notesDir, sizeof(directory_entry));
  memcpy(&dirBuffer[1], parentDir, sizeof(directory_entry));
  memcpy(&dirBuffer[2], someFile, sizeof(directory_entry));
  memcpy(&dirBuffer[3], bigFile, sizeof(directory_entry));

  blocksWritten = LBAwrite(dirBuffer, 1, notesDir->block_location);
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

  memset(strBuff, '\0', 100);
  strcpy(strBuff, someFileString);
  // strncpy(strBuff, "This is someFile.txt that has a little bit of data inside of it", 100);

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

  dirBuffer = realloc(dirBuffer, 2*vcb->block_size);
  if (dirBuffer == NULL) {
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

  // strncpy(strBuff, bigString, 700);
  memset(strBuff, '\0', 700);
  strcpy(strBuff, bigFileString);
  // strBuff[700 - 1] = '\0';
  memcpy(&dirBuffer[1], strBuff, 700);

  blocksWritten = LBAwrite(dirBuffer, 2, bigFile->block_location);
  if (blocksWritten < 2) {
    printf("%d %ld\n", blocksWritten, bigFile->block_location);
    perror("Error writing to disk");
    exit(EXIT_FAILURE);
  }

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

void *fs_malloc(size_t size, const char *failMsg) {
  // Allocates memory using malloc, and exits if it fails
  void* ptr = malloc(size);
  if (ptr == NULL) {
    printf("%s\n", failMsg);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

unsigned char *fs_malloc_buff(size_t size, uint64_t blockSize, const char *failMsg) {
  /* 
    Allocates a zero-initialized buffer whose size is evenly divisible by "blockSize".
    The size of this buffer is always greater than or equal to "size".
    Prints out "failMsg" if it fails to malloc the buffer.
  */
  uint64_t numberOfBlocks = getMinimumBlocks(size, blockSize);
  debug_print("fs_malloc num blocks: %ld\n", numberOfBlocks);
  debug_print("fs_malloc buff size: %ld\n", blockSize*numberOfBlocks);
  unsigned char *buffer = malloc(blockSize*numberOfBlocks);
  if (buffer == NULL) {
    printf("%s\n", failMsg);
    exit(EXIT_FAILURE);
  }

  memset(buffer, '\0', blockSize*numberOfBlocks);

  return buffer;
}

// Not tested yet. It's really wonky
void *fs_realloc(void** oldPtr, size_t newSize, unsigned char returnOldPtr, const char *failMsg) {
  /* 
    Reallocates memory using realloc
    If realloc fails, 
      it will return oldPtr if returnOldPtr is 1
    else, 
      it will exit the program
  */
  // void *temp = realloc(oldPtr, newSize);
  // if (temp == NULL) {
  //   free(temp);
  //   if (returnOldPtr == 0) {
  //     printf("%s\n", failMsg);
  //     exit(EXIT_FAILURE);
  //   } else {
  //     return *oldPtr;
  //   }
  // }

  // *oldPtr = &temp;
  // return oldPtr;

  void *temp;
  temp = realloc(*oldPtr, newSize);
  if (temp != NULL) {
    *oldPtr = temp;
    return oldPtr;
  } else {
    printf("Realloc fail, trying to free\n");
    free(temp);
    if (returnOldPtr == 0) {
      printf("%s\n", failMsg);
      exit(EXIT_FAILURE);
    } else {
      return oldPtr;
    }
  }
}

// unsigned char* fs_LBAread(size_t size, uint64_t blockSize, uint64_t lbaCount, uint64_t lbaPosition, const char* failMsg) {
//   /* Reads from disk using LBAread and checks for read errors */
//   uint64_t blocksRead = 0;
//   uint64_t blocksToRead = getMinimumBlocks(size, blockSize);
//   unsigned char *buffer = fs_malloc_buff(size, blockSize, "Unable to malloc buffer for fs_LBAread");
//   blocksRead = LBAread(buffer, lbaCount, lbaPosition);
//   if (blocksRead < blocksToRead) {
//     debug_print(
//       "LBAread() failure:\n"
//       "size: %ld\n"
//       "blockSize: %ld\n"
//       "lbaPosition: %ld\n"
//       "lbaCount: %ld\n"
//       "blocksRead: %ld\n"
//       "blocksToRead: %ld\n",
//       size,
//       blockSize,
//       lbaPosition,
//       lbaCount,
//       blocksRead,
//       blocksToRead
//     );
//     printf("%s\n", failMsg);
//     exit(EXIT_FAILURE);
//   }

//   return buffer;
// }

unsigned char* fs_LBAread(void *buffer, uint64_t lbaCount, uint64_t lbaPosition, const char* failMsg) {
  /* Reads from disk and stores into buffer. Aborts if it fails */
  int blocksRead = LBAread(buffer, lbaCount, lbaPosition);
  if (blocksRead < lbaCount) {
    printf("%s\n", failMsg);
    exit(EXIT_FAILURE);
  }
  return buffer;
}

void fs_LBAwrite(void *buffer, uint64_t lbaCount, uint64_t lbaPosition, const char *failMsg) {
  /* Writes buffer to disk and aborts if it fails */
  int blocksWritten = LBAwrite(buffer, lbaCount, lbaPosition);
  if (blocksWritten < lbaCount) {
    printf("%s\n", failMsg);
    exit(EXIT_FAILURE);
  }
}

uint64_t fs_getMinimumBlocks(uint64_t bytes, uint64_t blockSize) {
  /* Returns the minimum number of lba blocks needed to fit "bytes" into */
  if (blockSize == 0) {
    printf("Division by zero error\n");
    exit(EXIT_FAILURE);
  }

  uint64_t blocksNeeded = bytes / blockSize;
  uint64_t remainder = bytes % blockSize;
  if (remainder > 0) {
    return blocksNeeded + 1;
  } else {
    return blocksNeeded;
  }
}

uint64_t fs_getMinimumBytes(uint64_t bytes, uint64_t blockSize) {
  /* Returns the byte size of the minimum number of lba blocks needed to fit "bytes" into*/
  if (blockSize == 0) {
    printf("Division by zero error\n");
    exit(EXIT_FAILURE);
  }

  uint64_t blocksNeeded = bytes / blockSize;
  uint64_t remainder = bytes % blockSize;
  if (remainder > 0) {
    return (blocksNeeded + 1)*blockSize;
  } else {
    return blocksNeeded*blockSize;
  }
}

char* concatStrings(char* s1, const char* s2, size_t size) {
  /*
    Concatenates s2 to s1. The concatenated string is returned by s1
  */
  // 0x0140;
	// Returns null if s1 + s2 exceeds capacity of s1
	if (strlen(s1) + strlen(s2) <= size) {
		memcpy(s1 + strlen(s1), s2, strlen(s2));
    // debug_print("New string s1: %s\n", s1);
	} else {
	  return NULL;
	}
}