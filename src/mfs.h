/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Further Modified By: Mohammed Deeb, Miguel Antonio Logarta, Abdarrahman Ayyaz, Avitaj Singh Bhangoo
* Student IDs: 922875899, 923062683, 922859909, 921303783
* Project: Basic File System
*
* File: mfs.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
*	There are also some extra utility functions defined in mfs.h
* to help simplify memory allocation and pathname parsing
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"
#include "partition.h"

#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#include <sys/stat.h>

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

#define MAX_PATH 256 // Maximum file path length, including null character

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
{
    unsigned short d_reclen;    /* length of this record */ // Note that this is not the file size, but rather the size of the struct itself
    unsigned char fileType;    	/* Holds values FT_REGFILE, FT_DIRECTORY, or FT_LINK
																		Determines if the file is a regular file, directory, or symlink
																*/
    char d_name[256]; 			/* filename max filename is 255 characters */

		/* Any other attributes your file system needs */
		uint64_t block_location;	// Starting LBA block
		size_t file_size;					// Size of file/directory in bytes
		time_t date_created;			
		time_t last_modified;
};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct
{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	unsigned short  d_reclen;		/* length of this record. Note that this is not the file size, but rather the size of fdDir */
	unsigned short	dirEntryPosition;	/* which directory entry position, like file pos */
	directory_entry *directory; // This is our loaded directory
	unsigned char* dirContent;
	struct fs_diriteminfo * di;		/* Pointer to the structure you return from read */
	char absolutePath[MAX_PATH];	// Stores our absolute path to this directory
} fdDir;

// Global uninitialized variables to store our state
extern fdDir* g_fs_cwd;	// Stores our current working directory

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
fdDir * fs_createFdDir(directory_entry *directoryContents, const char *absolutePath, char* dirContent);			// Reads directory entry and returns it as fdDir
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file


// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	uint64_t st_block_location;
	int 			st_location;
	/* add additional attributes here for your file system */
	unsigned char st_filetype;	/* Indicates the type of directory entry */
};

int fs_stat(const char *path, struct fs_stat *buf);

/* Utility functions */
/* Manually writes files to the disk. For testing purposes only */
int writeTestFiles();
uint64_t getMinimumBlocks(uint64_t bytes, uint64_t blockSize);
uint64_t fs_getMinimumBlocks(uint64_t bytes, uint64_t blockSize);
uint64_t fs_getMinimumBytes(uint64_t bytes, uint64_t blockSize);
char* fs_formatPathname(const char* oldPath, const char* newPath);
char* fs_getLastToken(const char* path);
char* fs_getAbsolutePath(const char* path);

/* Wrapper functions for memory allocation */
void *fs_malloc(size_t size, const char *failMsg);
unsigned char *fs_malloc_buff(size_t size, uint64_t blockSize, const char *failMsg);
void *fs_realloc(void** oldPtr, size_t newSize, unsigned char returnOldPtr, const char *failMsg);

/* Wrapper functions for LBAread and LBAwrite*/
unsigned char* fs_LBAread(void *buffer, uint64_t lbaCount, uint64_t lbaPosition, const char* failMsg);
void fs_LBAwrite(void *buffer, uint64_t lbaCount, uint64_t lbaPosition, const char *failMsg);

char* concatStrings(char* s1, const char* s2, size_t size);

#endif

