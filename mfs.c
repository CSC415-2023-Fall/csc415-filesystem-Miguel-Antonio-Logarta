#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    for (int i = sizeof(directory_entry); i < currDir->file_size; i += sizeof(directory_entry)) {
      directory_entry* subfolder = (directory_entry*)malloc(sizeof(directory_entry));
      memcpy(subfolder, buffer + i, sizeof(directory_entry));

      if (subfolder->is_directory == 0) {
        if (strcmp(subfolder->name, token) == 0) {
          // Match found, set current folder to subfolder
          realloc(buffer, subfolder->file_size);
          blocksRead = LBAread(buffer, 1, subfolder->block_location);
          memcpy(currDir, buffer, sizeof(directory_entry));
        }
      }
    }

    token = strtok(NULL, "/");
  }

  printf("Last folder: %s\n", lastToken);

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

struct fs_diriteminfo *fs_readdir(fdDir *dirp);
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
  
}
