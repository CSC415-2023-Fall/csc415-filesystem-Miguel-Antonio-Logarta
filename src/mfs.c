#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fsLow.h"
#include "mfs.h"
#include "debug.h"

// Store out current working directory in memory
fdDir *g_fs_cwd = NULL;

int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Test for multiple inputs and conditions
// relative/path/to/file.txt
// ./dot/notation/to/indicate/current/path.c
// ../double/dot/to/go/up/to/parents
// /root/slash/to/indicate/absolute path
// /root/slash/../slash/to
// /root/slash/./to
fdDir *fs_opendir(const char *pathname) {
  /*
          We receive an absolute path from pathname, perform a DFS search to
     find the directory, then we return a pointer to that directory
  */
  /* TODO: 
      - Move "checking if pathname is valid" in a different function
      - Test relative paths
      - Test .. in pathname 
      - Test .  in pathname
  */
  if (pathname == NULL) {
    printf("Pathname is NULL\n");
    return NULL;
  }
  printf("---------------\n%s\n", pathname);

  // If user passes in "" as path, just return root or current working directory
  if (pathname[0] == '\0') {
    printf("Pathname is blank");
    return NULL;
  }

  if (strlen(pathname) + 1 > MAX_PATH) {
    printf("Pathname exceeds MAX_PATH\n");
    return NULL;
  }

  // Variables for path parsing
  unsigned int isAbsolutePath;
  char *path;
  char *token;

  // Variables to read file directories
  VCB *fsVCB;
  unsigned char *buffer;
  directory_entry *currentDirectory;
  unsigned char *directoryContents;
  uint64_t blocksRead;
  uint64_t blocksWritten;

  // Copy pathname
  path = malloc(MAX_PATH);
  if (path == NULL) {
    printf("Error allocating memory for path\n");
    exit(EXIT_FAILURE);
  }

  strncpy(path, pathname, MAX_PATH);

  // Check if pathname is absolute or relative path
  if (path[0] == '/') {
    isAbsolutePath = 1;
  } else {
    isAbsolutePath = 0;
  }

  // Get VCB
  fsVCB = fs_getvcb();
  if (fsVCB == NULL) {
    printf("There was an error allocating the VCB\n");
    exit(EXIT_FAILURE);
  }

  // Load root directory or current working directory
  if (isAbsolutePath == 1) {
    buffer = malloc(fsVCB->DE_length * fsVCB->block_size);
    if (buffer == NULL) {
      printf("Error allocating buffer\n");
      exit(EXIT_FAILURE);
    }

    blocksRead = LBAread(buffer, fsVCB->DE_length, fsVCB->DE_start);
    if (blocksRead < fsVCB->DE_length) {
      printf("Error reading root directory\n");
      exit(EXIT_FAILURE);
    }

    currentDirectory = malloc(sizeof(directory_entry));
    if (currentDirectory == NULL) {
      printf("Error with malloc for current directory\n");
      exit(EXIT_FAILURE);
    }

    memcpy(currentDirectory, buffer, sizeof(directory_entry));
  } else {
    if (g_fs_cwd == NULL) {
      printf("Current working directory has not been initialized yet!\n");
      free(path);
      free(fsVCB);
      return NULL;
    } else {
      // Set current directory to cwd
      currentDirectory = malloc(sizeof(directory_entry));
      
      buffer = malloc(fsVCB->DE_length * fsVCB->block_size);
      if (buffer == NULL) {
        printf("Error allocating buffer\n");
        exit(EXIT_FAILURE);
      }

      if (currentDirectory == NULL) {
        printf("Error with malloc for current directory\n");
        exit(EXIT_FAILURE);
      }

      memcpy(currentDirectory, g_fs_cwd->directory, sizeof(directory_entry));
    }
  }

  token = strtok(path, "/");
  int subfolderFound = 0;

  if (strcmp(pathname, "/") == 0) {
    subfolderFound = 1;
  }

  while (token != NULL) {
    // TODO: When the current token is "." skip this token
    subfolderFound = 0;
    // int subfolderFound = 0;
    // Read contents of current directory from disk
    printf("Current directory info: %ld, %ld: \n", currentDirectory->file_size, fsVCB->block_size);
    int minimumBlocks = getMinimumBlocks(currentDirectory->file_size, fsVCB->block_size);
    uint64_t bytesToLoad = fsVCB->block_size*minimumBlocks;
    buffer = realloc(buffer, bytesToLoad);
    memset(buffer, '\0', bytesToLoad);
    blocksRead = LBAread(buffer, minimumBlocks, currentDirectory->block_location);
    if (blocksRead < minimumBlocks) {
      printf("Error reading directory: %s", currentDirectory->name);
      return NULL;
    }

    // Load from buffer into directory contents for reading
    printf("min blocks: %d, bytesToLoad: %ld, blocksRead: %ld\n", minimumBlocks, bytesToLoad, blocksRead);
    directoryContents = malloc(currentDirectory->file_size);
    if (directoryContents == NULL) {
      printf("Error malloc'ing directoryContents");
      exit(EXIT_FAILURE);
    }
    memcpy(directoryContents, buffer, currentDirectory->file_size);

    for (int i = sizeof(directory_entry); i < currentDirectory->file_size; i += sizeof(directory_entry)) {
      directory_entry* subfolder = malloc(sizeof(directory_entry));
      if (subfolder == NULL) {
        printf("Error malloc'ing subfolder\n");
        exit(EXIT_FAILURE);
      }
      memcpy(subfolder, directoryContents + i, sizeof(directory_entry));

      printf("|_%s\n", subfolder->name);

      if (subfolder->is_directory == 1 && strcmp(subfolder->name, token) == 0) {
        printf("Match found %s == %s\n", subfolder->name, token);
        
        // Enter directory
        memset(currentDirectory, '\0', sizeof(directory_entry));
        memcpy(currentDirectory, subfolder, sizeof(directory_entry));
        subfolderFound = 1;
      }

      free(subfolder);

      if (subfolderFound == 1) {
        // No need to search the rest of the folders
        break;
      }
    }

    if (subfolderFound == 0) {
      printf("Folder not found: %s\n", token);
    }

    free(directoryContents);
    token = strtok(NULL, "/");

    if (subfolderFound == 0) {
      break;
    }
  }

  fdDir* newFdDir = malloc(sizeof(fdDir));
  if (subfolderFound == 1) {
    newFdDir->d_reclen = currentDirectory->file_size;
    // newFdDir->dirEntryPosition = currentDirectory->block_location;

    // Pointer to our di offset
    newFdDir->dirEntryPosition = 0;
    // TODO: Do a memcpy instead
    // newFdDir->directory = currentDirectory;
    newFdDir->directory = malloc(sizeof(directory_entry));
    memcpy(newFdDir->directory, currentDirectory, sizeof(directory_entry));
    // TODO: assign return value of fsreaddir() to this 
    newFdDir->di = NULL;
    strncpy(newFdDir->absolutePath, pathname, MAX_PATH);
    printf("Folder to return: %s\n", currentDirectory->name);
  }

  free(path);
  free(fsVCB);
  free(buffer);
  free(currentDirectory);

  if (subfolderFound == 1) {
    return newFdDir;
  } else {
    free(newFdDir);
    return NULL;
  }
}

fdDir *fs_opendirV3(const char *pathname) {
  /*
          We receive an absolute path from pathname, perform a DFS search to
     find the directory, then we return a pointer to that directory
  */
  /* TODO: 
      - Move "checking if pathname is valid" in a different function
      - Test relative paths
      - Test .. in pathname 
      - Test .  in pathname
  */
  if (pathname == NULL) {
    printf("Pathname is NULL\n");
    return NULL;
  }

  // // If user passes in "" as path, just return root or current working directory
  // if (pathname[0] == '\0') {
  //   printf("Pathname is blank");
  //   return NULL;
  // }

  if (strlen(pathname) + 1 > MAX_PATH) {
    printf("Pathname exceeds MAX_PATH\n");
    return NULL;
  }

  // Variables for path parsing
  unsigned int isAbsolutePath;
  char *path;
  char *token;

  // Variables to read file directories
  VCB *fsVCB;
  unsigned char *buffer;
  directory_entry *currentDirectory;
  unsigned char *directoryContents;
  uint64_t blocksRead;
  uint64_t blocksWritten;

  // Copy pathname
  path = malloc(MAX_PATH);
  if (path == NULL) {
    printf("Error allocating memory for path\n");
    exit(EXIT_FAILURE);
  }

  strncpy(path, pathname, MAX_PATH);

  // Check if pathname is absolute or relative path
  if (path[0] == '/') {
    isAbsolutePath = true;
  } else {
    isAbsolutePath = false;
  }

  // Get VCB
  fsVCB = fs_getvcb();

  // Load root directory or current working directory
  if (isAbsolutePath == true) {
    buffer = fs_malloc_buff(sizeof(VCB), fsVCB->block_size, "Unable to malloc buffer");
    buffer = fs_LBAread(buffer, 1, 0, "Unable to read root");
    currentDirectory = fs_malloc(sizeof(directory_entry), "Unable to malloc current directory");
    memcpy(currentDirectory, buffer, sizeof(directory_entry));
  } else {
    if (g_fs_cwd == NULL) {
      printf("Current working directory has not been initialized yet!\n");
      free(path);
      free(fsVCB);
      return NULL;
    } else {
      // Set current directory to cwd
      currentDirectory = fs_malloc(sizeof(directory_entry), "Unable to malloc cwd");
      // buffer = fs_malloc_buff(sizeof(VCB), fsVCB->block_size, "Unable to malloc buffer");
      memcpy(currentDirectory, g_fs_cwd->directory, sizeof(directory_entry));
    }
  }

  token = strtok(path, "/");
  int subfolderFound = false;

  if (strcmp(pathname, "/") == 0) {
    subfolderFound = true;
  }

  while (token != NULL) {
    // TODO: When the current token is "." skip this token
    subfolderFound = 0;
    // int subfolderFound = 0;
    // Read contents of current directory from disk
    printf("Current directory info: %ld, %ld: \n", currentDirectory->file_size, fsVCB->block_size);
    int minimumBlocks = getMinimumBlocks(currentDirectory->file_size, fsVCB->block_size);
    uint64_t bytesToLoad = fsVCB->block_size*minimumBlocks;
    buffer = realloc(buffer, bytesToLoad);
    memset(buffer, '\0', bytesToLoad);
    blocksRead = LBAread(buffer, minimumBlocks, currentDirectory->block_location);
    if (blocksRead < minimumBlocks) {
      printf("Error reading directory: %s", currentDirectory->name);
      return NULL;
    }

    // Load from buffer into directory contents for reading
    printf("min blocks: %d, bytesToLoad: %ld, blocksRead: %ld\n", minimumBlocks, bytesToLoad, blocksRead);
    directoryContents = malloc(currentDirectory->file_size);
    if (directoryContents == NULL) {
      printf("Error malloc'ing directoryContents");
      exit(EXIT_FAILURE);
    }
    memcpy(directoryContents, buffer, currentDirectory->file_size);

    for (int i = sizeof(directory_entry); i < currentDirectory->file_size; i += sizeof(directory_entry)) {
      directory_entry* subfolder = malloc(sizeof(directory_entry));
      if (subfolder == NULL) {
        printf("Error malloc'ing subfolder\n");
        exit(EXIT_FAILURE);
      }
      memcpy(subfolder, directoryContents + i, sizeof(directory_entry));

      printf("|_%s\n", subfolder->name);

      if (subfolder->is_directory == 1 && strcmp(subfolder->name, token) == 0) {
        printf("Match found %s == %s\n", subfolder->name, token);
        
        // Enter directory
        memset(currentDirectory, '\0', sizeof(directory_entry));
        memcpy(currentDirectory, subfolder, sizeof(directory_entry));
        subfolderFound = 1;
      }

      free(subfolder);

      if (subfolderFound == 1) {
        // No need to search the rest of the folders
        break;
      }
    }

    if (subfolderFound == 0) {
      printf("Folder not found: %s\n", token);
    }

    free(directoryContents);
    token = strtok(NULL, "/");

    if (subfolderFound == 0) {
      break;
    }
  }

  fdDir* newFdDir = malloc(sizeof(fdDir));
  if (subfolderFound == 1) {
    newFdDir->d_reclen = currentDirectory->file_size;
    // newFdDir->dirEntryPosition = currentDirectory->block_location;

    // Pointer to our di offset
    newFdDir->dirEntryPosition = 0;
    // TODO: Do a memcpy instead
    // newFdDir->directory = currentDirectory;
    newFdDir->directory = malloc(sizeof(directory_entry));
    memcpy(newFdDir->directory, currentDirectory, sizeof(directory_entry));
    // TODO: assign return value of fsreaddir() to this 
    newFdDir->di = NULL;
    strncpy(newFdDir->absolutePath, pathname, MAX_PATH);
    printf("Folder to return: %s\n", currentDirectory->name);
  }

  free(path);
  free(fsVCB);
  free(buffer);
  free(currentDirectory);

  if (subfolderFound == 1) {
    return newFdDir;
  } else {
    free(newFdDir);
    return NULL;
  }
}

fdDir *fs_createFdDir(directory_entry *de, const char *absolutePath) {
  fdDir *returnFdDir = fs_malloc(sizeof(fdDir), "Unable to malloc returnFdDir");
  returnFdDir->directory = fs_malloc(sizeof(directory_entry), "Unable to malloc returnFdDir->directory");
  // returnFdDir->absolutePath = fs_malloc(MAX_PATH, "Unable to malloc returnFdDir->absolutePath");
  
  returnFdDir->d_reclen = de->file_size;
  returnFdDir->dirEntryPosition = 0;
  returnFdDir->di = NULL;
  strncpy(returnFdDir->absolutePath, absolutePath, MAX_PATH);
  memcpy(returnFdDir->directory, de, sizeof(directory_entry));

  return returnFdDir;
}

fdDir* fs_opendirV2(const char *pathname) {
  /*
    More improved version of fs_opendir with cleaner mode readable code.
    Can now properly handle . and .. 

    TODO: Since b_read() has not been done yet, all directories can only support up to 
    512/64 = 8 directories, including itself. 
  */
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
    debug_print("%s is an absolute path\n", pathname);

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

    absolutePath = fs_malloc(MAX_PATH, "Error allocating memory for absolutePath\n");
    strncpy(absolutePath, "/", MAX_PATH);

    debug_print("%s is initialized\n", absolutePath);
  } else {
    debug_print("%s is a relative path\n", pathname);

    // Load current working directory as starting directory
    buffer = fs_malloc_buff(g_fs_cwd->d_reclen, vcb->block_size, "Error getting root");
    buffer = fs_LBAread(buffer, 1, g_fs_cwd->directory->block_location, "Unable to read root");

    currentDirectory = fs_malloc(sizeof(directory_entry), "Error with malloc for current directory");
    memcpy(currentDirectory, buffer, sizeof(directory_entry));
    
    directoryContents = fs_malloc(currentDirectory->file_size, "Unable to malloc directory contents");
    memcpy(directoryContents, buffer, currentDirectory->file_size);

    path = fs_malloc(MAX_PATH, "Error allocating memory for path\n");
    strncpy(path, pathname, MAX_PATH);

    absolutePath = fs_malloc(MAX_PATH, "Error allocating memory for absolutePath\n");
    strncpy(absolutePath, g_fs_cwd->absolutePath, MAX_PATH);

    debug_print("Relative path %s is initialized\n", absolutePath);
  }
  
  subfolderFound = true;
  token = strtok(path, "/");
  while (token != NULL) {
    debug_print("current token: %s\n", token);
    
    subfolderFound = false;   // Reset flag

    // Skip "."" in path
    if (strcmp(token, ".") == 0) {
      continue;
    }

    debug_print("%s\n", currentDirectory->name);

    // Loop through every subfolder of the current folder
    for (int i = sizeof(directory_entry); i < currentDirectory->file_size; i += sizeof(directory_entry)) {
      directory_entry* subfolder = fs_malloc(sizeof(directory_entry), "Unable to malloc subfolder");
      memcpy(subfolder, directoryContents + i, sizeof(directory_entry));

      debug_print("|_%s\n", subfolder->name);

      if (subfolder->is_directory == 1 && strcmp(subfolder->name, token) == 0) {
        debug_print("Subfolder found\n");
        subfolderFound = true;

        // Set current directory to this subfolder
        // Load current working directory as starting directory

        // Change to realloc
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
        
        if (strcmp(token, "..") != 0) {
          concatStrings(absolutePath, token, MAX_PATH);
          debug_print("%s\n", absolutePath);
        }

        free(subfolder);
        break;
      }

      free(subfolder);
    }

    if (subfolderFound == false) {
      break;
    }

    token = strtok(NULL, "/");
  }

  if (subfolderFound == true) {
    debug_print("Path is valid, returning fd\n");
    returnDir = fs_createFdDir(currentDirectory, absolutePath);
  } else {
    debug_print("Path is not valid, returning NULL\n");
    returnDir = NULL;
  }

  free(path);
  free(absolutePath);
  free(vcb);
  free(buffer);
  free(currentDirectory);
  free(directoryContents);

  return returnDir;
  /*
    If pathname was an absolute path
      Set starting/current directory as root
    If pathname was a relative path
      Set starting/current directory as cwd
  */
  // if (isAbsolutePath == true) {
  //   debug_print("This is an absolute path!\n");
  //   buffer = fs_malloc_buff(
  //     vcb->DE_length*vcb->block_size, 
  //     vcb->block_size, 
  //     "Error getting root");
    
  //   currentDirectory = fs_malloc(sizeof(directory_entry), "Error with malloc for current directory");

  //   buffer = fs_LBAread(buffer, 1, vcb->DE_start, "Unable to read root");

  //   memcpy(currentDirectory, buffer, sizeof(directory_entry));
  //   strncpy(absolutePathCopy, "/", MAX_PATH);
  //   debug_print("currdir: %s\n", currentDirectory->name);
  //   debug_print("end of malloc stuff\n");
  // } else {
  //   if (g_fs_cwd == NULL) {
  //     printf("Current working directory has not been initialized yet!\n");
  //     free(path);
  //     free(absolutePathCopy);
  //     free(vcb);
  //     return NULL;
  //   } else {
  //     // TODO: Set current directory to cwd (Untested)
  //     currentDirectory = fs_malloc(sizeof(directory_entry), "Error with malloc for current directory");
      
  //     // int blocksToRead = getMinimumBlocks(g_fs_cwd->d_reclen, fsVCB->block_size);
      
  //     memcpy(currentDirectory, g_fs_cwd->directory, sizeof(directory_entry));
  //     strncpy(absolutePathCopy, g_fs_cwd->absolutePath, MAX_PATH);
  //   }
  // }

  // // Edge cases:
  // /*
  //   pathname = .....    // Say dir not found
  //   pathname = /////////  // Return root dir
  //   pathname = ""         // Return root dir
  // */
  // token = strtok(path, "/");

  // // TODO: Delete this if else block when done
  // if (token != NULL) {
  //   debug_print("token = %s, strlen(pathname) = %ld, strlen(token) = %ld\n", token, strlen(pathname), strlen(token));
  // } else {
  //   // If token is null
  //   /*
  //     It could be a it is either blank, or just a sequence of / characters.
  //     It will skip the while loop
  //   */
  //   debug_print("Either blank or /////");
  //   debug_print("token is null, strlen(pathname) = %ld\n", strlen(pathname));
  // }

  // while (token != NULL) {
  //   debug_print("current token: %s\n", token);

  //   // Skip "."" in path
  //   if (strcmp(token, ".") == 0) {
  //     continue;
  //   }

  //   // Load contents of directory from disk
  //   // buffer = fs_malloc_buff(currentDirectory->file_size, vcb->block_size, "Unable to malloc directory buffer");
  //   // fs_LBAread(buffer, 1, currentDirectory->block_location, "Unable to read directory into disk");
  //   // directoryContents = fs_malloc(currentDirectory->file_size, "Unable to malloc directoryContents");
  //   // memcpy(directoryContents, buffer, currentDirectory->file_size);

  //   // // Check if subdirectory exists in current directory
  //   // int subfolderFound = false;

  //   // // This for loop is having heap overflow issues
  //   // for (int i = sizeof(directory_entry); i < currentDirectory->file_size; i += sizeof(directory_entry)) {
  //   //   directory_entry* subfolder = fs_malloc(sizeof(directory_entry), "Unable to malloc subfolder");
  //   //   memcpy(subfolder, directoryContents + i, sizeof(directory_entry));

  //   //   debug_print("|_%s\n", subfolder->name);

  //   //   // if (subfolder->is_directory == 1 && strcmp(subfolder->name, token) == 0) {
  //   //   //   debug_print("Match found %s == %s\n", subfolder->name, token);
        
  //   //   //   // Enter directory
  //   //   //   memset(currentDirectory, '\0', sizeof(directory_entry));
  //   //   //   memcpy(currentDirectory, subfolder, sizeof(directory_entry));

  //   //   //   // Update absolute path
  //   //   //   // This is okay since we already checked if pathname > MAX_PATH
  //   //   //   // strncpy(absolutePathCopy + strlen(absolutePathCopy) + 1, currentDirectory->name, strlen(currentDirectory->name));

  //   //   //   subfolderFound = true;
  //   //   //   free(subfolder);
  //   //   //   break;
  //   //   // }

  //   //   free(subfolder);
  //   // }

  //   // if (subfolderFound == false) {
  //   //   // /Home/Config/misc triggers this. fsVCB is invalid.
  //   //   d_printVCB(fsVCB);

  //   //   // If it entered here, then no match was found
  //   //   free(fsVCB);
  //   //   free(path);
  //   //   free(absolutePathCopy);
  //   //   free(buffer);
  //   //   free(currentDirectory);
  //   //   free(directoryContents);
  //   //   return NULL;
  //   // }

  //   token = strtok(NULL, "/");
  // }

  // fdDir *returnDir = fs_createFdDir(currentDirectory, absolutePathCopy);
  
  // free(path);
  // free(absolutePathCopy);
  // free(vcb);
  // free(buffer);
  // free(currentDirectory);
  // free(directoryContents);
  
  // return returnDir;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {

  printf("Dir entry position and dirp reclen: %d > %d\n", dirp->dirEntryPosition, dirp->d_reclen);
  if (dirp->dirEntryPosition > dirp->d_reclen) {
    printf("Dir entry position greater than dirp: %d > %d\n", dirp->dirEntryPosition, dirp->d_reclen);
    return NULL;
  }

  // Variables area
  VCB* fsVCB;
  unsigned char* buffer;
  directory_entry* currentDE;
  struct fs_diriteminfo* DEInfo;
  uint64_t blocksRead;
  // uint64_t readPosition;
  uint64_t blocksToRead;

  // Read the vcb to get size
  fsVCB = fs_getvcb();

  blocksToRead = getMinimumBlocks(dirp->d_reclen, fsVCB->block_size);
  // readPosition = fsVCB->block_size*dirp->directory->block_location;

  buffer = malloc(fsVCB->block_size*blocksToRead);
  if (buffer == NULL) {
    printf("Failure to malloc buffer\n");
    exit(EXIT_FAILURE);
  }
  
  blocksRead = LBAread(buffer, blocksToRead, dirp->directory->block_location);
  if (blocksRead < blocksToRead) {
    printf("Failure to read directory entry\n");
    exit(EXIT_FAILURE);
  }

  currentDE = malloc(sizeof(directory_entry));
  if (currentDE == NULL) {
    printf("Failure to malloc currentDE\n");
    exit(EXIT_FAILURE);
  }

  DEInfo = malloc(sizeof(struct fs_diriteminfo));
  if (DEInfo == NULL) {
    printf("Failure to malloc DEInfo\n");
    exit(EXIT_FAILURE);
  }

  memcpy(currentDE, buffer + dirp->dirEntryPosition, sizeof(directory_entry));

  DEInfo->d_reclen = currentDE->file_size;
  DEInfo->fileType = currentDE->is_directory;
  strncpy(DEInfo->d_name, currentDE->name, MAX_PATH);

  printf("Inside readdir: %d %d %s\n", DEInfo->d_reclen, DEInfo->fileType, DEInfo->d_name);

  dirp->dirEntryPosition += sizeof(directory_entry);

  free(fsVCB);
  free(buffer);
  free(currentDE);

  return DEInfo;
}

int fs_closedir(fdDir *dirp) {
  /* Frees fdDir struct from memory */
  free(dirp->directory);
  free(dirp->di);
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
    return NULL;
  }

  // Pathname is basically our buffer to fill, we also return the pointer to this buffer
  printf("Size: %ld, strlen: %ld\n", size, strlen(g_fs_cwd->absolutePath) + 1);
  if (pathname == NULL || size < strlen(g_fs_cwd->absolutePath) + 1) {
    int absolutePathSize = strlen(g_fs_cwd->absolutePath) + 1;
    pathname = realloc(pathname, absolutePathSize);
    if (pathname == NULL) {
      perror("Reallocation of pathname failed!");
      exit(EXIT_FAILURE);
    }
    strncpy(pathname, g_fs_cwd->absolutePath, absolutePathSize);

    for (int i = 0; i < absolutePathSize; i++) {
      printf("%d, %c\n", i, pathname[i]);
    }
  } else {
    strncpy(pathname, g_fs_cwd->absolutePath, size);
  }
  
  return pathname;
}


int fs_setcwd(char *pathname) {
  // This is the equivalent of the linux chdir or cd
  fdDir* newCwd;
  newCwd = fs_opendirV2(pathname);
  
  if (newCwd == NULL) {
    printf("Error reallocating new g_fs_cwd\n");
    return -1;
  } else {
    g_fs_cwd = realloc(g_fs_cwd, sizeof(fdDir));
    if (g_fs_cwd == NULL) {
      printf("Error reallocating g_fs_cwd!\n");
      exit(EXIT_FAILURE);
    }

    memcpy(g_fs_cwd, newCwd, sizeof(fdDir));
    free(newCwd);

    return 0;
  }
}

int fs_isFile(char *filename) {
  return 0;
}

int fs_isDir(char *pathname) {
  return 0;
}

int fs_delete(char *filename); // removes a file

int fs_stat(const char *path, struct fs_stat *buf) {
  /* 
    Fetches information about a file/directory via the fs_stat struct
    Returns -1 on failure
    Returns 0 on success
  */
 
  fdDir *file = fs_opendir(path);
  if (file == NULL) {
    return -1;
  } else {
    VCB* vcb = fs_getvcb();

    buf->st_size = file->di->d_reclen;
    buf->st_blksize = vcb->block_size;
    buf->st_blocks = getMinimumBlocks(file->di->d_reclen, vcb->block_size);
    buf->st_accesstime = time(0);
    buf->st_modtime = file->directory->last_modified;
    buf->st_createtime = file->directory->date_created;

    free(vcb);
    free(file);

    return 0;
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

  notesDir->block_location = 157;
  strncpy(notesDir->name, "Notes", sizeof(notesDir->name) / sizeof(notesDir->name[0])); 
  notesDir->is_directory = 1; notesDir->file_size = sizeof(directory_entry)*4;    // We are inserting 2 files into the directory 
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

char* concatStrings(char* s1, char* s2, size_t size) {
	// Returns null if s1 + s2 exceeds capacity of s1
	if (strlen(s1) + strlen(s2) <= size) {
		memcpy(s1 + strlen(s1), s2, strlen(s2));
    debug_print("New string s1: %s\n", s1);
	} else {
		return NULL;
	}
}