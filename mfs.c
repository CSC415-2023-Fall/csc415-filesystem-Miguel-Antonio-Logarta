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

fdDir * fs_opendir(const char *pathname) {
	/*
		We receive an absolute path from pathname, perform a DFS search to find the directory,
		then we return a pointer to that directory
	*/

  // TODO: tokenize the pathname with "/"

  // Open VCB to get root dir
  // printf("fs_open called\n");
  VCB* fs_vcb = getVCB();
  printf("%ld\n", fs_vcb->DE_start);

  // TODO: We're going to need to use b_read() to get the directory
  // because what if the root directory spans more than 1 block?
  // printf("length, %ld, block size %ld = %ld\n", fs_vcb->DE_length, fs_vcb->block_size, fs_vcb->DE_length*fs_vcb->block_size);
  // unsigned char* buffer = malloc(fs_vcb->DE_length*fs_vcb->block_size);

  // Have a buffer with some extra available bits to prevent heap overflow
  unsigned char* buffer = malloc(fs_vcb->DE_length*fs_vcb->block_size+MINBLOCKSIZE);
  if (buffer == NULL) {
    printf("Couldn't allocate buffer");
    return NULL;
  }

  printf("Malloced buffer %d\n", fs_vcb->DE_length*fs_vcb->block_size);
  printf("Length before %ld, start offset: %ld\n", fs_vcb->DE_length, fs_vcb->DE_start);

  // memcpy(DE_blocksToRead, fs_vcb->DE_length, sizeof(uint64_t));
  // memcpy(DE_start, fs_vcb->DE_start, sizeof(uint64_t));
  // printf("Length before %ld, start offset: %ld\n", DE_blocksToRead, DE_start);
  // LBAread is changing the values of fs_vcb for some reason, I'm going to make a copy of teh variables instead
  
  printVCB(fs_vcb);
  // int blocksRead = 0;

  // There has to be some memory corruption going on. LBA read is probably overwriting some bits in memory
  
  // LBAread right here, right down here you see, arggghhh. If you give it a buffer of exactly 512 bytes, it will
  // run over and start writing to other parts of memory it shouldn't even be touching.
  // int blocksRead = LBAread(buffer, 1, 154); // <- this function is evil ;-;. It's changing the variables inside fs_vcb and I don't know how it's doing that
  // int blocksRead = LBAread(buffer, fs_vcb->DE_length, 154);
  // What the? Why does fs_vcb->DE_start change even though we don't pass it to lba read?
  // The name of the vcb doesn't matter. It maybe has to do something with the buffer
  printVCB(fs_vcb);
  int blocksRead = 0;
  printf("Blocks read: %ld < %ld?\n", blocksRead, fs_vcb->DE_length);
  printf("Length after %ld, start offset: %ld\n", fs_vcb->DE_length, fs_vcb->DE_start);
  // if (blocksRead < fs_vcb->DE_length) {
  //   printf("There is an error getting the Root directory\n");
  //   return NULL;
  // }

  // printf("Malloced used LBA read\n");

  // Right now, we know that our root dir is only one block large, however, this will fail
  // once we start putting more directories and files inside root dir.
  // printf("root dir length %d then size %ld\n", fs_vcb->DE_length, fs_vcb->DE_length*sizeof(directory_entry));

  // There is an issue down here
  // directory_entry *rootDir = (directory_entry*)malloc(3*sizeof(directory_entry));
  // directory_entry * rootDir;
  // rootDir = (directory_entry*)malloc(sizeof(directory_entry));
  
  // printf("Malloced Root dir\n");

  // memcpy(rootDir, buffer, fs_vcb->DE_length*sizeof(directory_entry));
  
  // Read the directory entry, check if it is a file or a folder
  // We are going to scroll through the buffer 64 bytes at a time,
  // for (int i = 0; i < fs_vcb->DE_length*fs_vcb->block_size; i += sizeof(directory_entry)) {
  //   directory_entry* currDir = (directory_entry*)malloc(sizeof(directory_entry));
  //   memcpy(currDir, buffer + i, sizeof(directory_entry));
  //   printf("currdir: %s\n", currDir->name);
  //   free(currDir);
  // }

  // Now because of lbaread, it's permanently messed up
  // fs_vcb = getVCB();
  printVCB(fs_vcb);
  
  // printf("curr dirrrrrrr: %s\n", rootDir[0].name);
  // int bytesRemaining = 0;
  // while (bytesRead < )
  // // for (int i = 0; i < fs_vcb->block_size; i += sizeof())
  // if (strcmp(rootDir[0].name, pathname) == 0) {
  //   printf("Pathname matches, directory found\n");
  // }

  // Traverse root dir until we find path

  // fdDir* newDir = (fdDir*)malloc(sizeof(fdDir));
  // return newDir;
  free(buffer);

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
