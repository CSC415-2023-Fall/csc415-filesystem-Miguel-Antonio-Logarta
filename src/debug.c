/**************************************************************
* Class:  CSC-415-02 Fall 2023
* Names: Mohammed Deeb, Miguel Antonio Logarta, Abdarrahman Ayyaz, Avitaj Singh Bhangoo
* Student IDs: 922875899, 923062683, 922859909, 921303783
* GitHub Name:
* Group Name: Byte Builders
* Project: Basic File System
*
* File: debug.c
*
* Description: Implementation of debug routines to help log information
*               about the file system
*
**************************************************************/


#include <sys/types.h>
#include <stdio.h>
#include "debug.h"

void d_printVCB(VCB* vcb) {
#ifdef DEBUG
	printf(
    "Printing VCB\n"
    "magic_signature: %ld\n"
    "volume_size: %ld\n"
    "block_size: %ld\n"
    "num_blocks: %ld\n"
    "FAT_start: %ld\n"
    "FAT_length: %ld\n"
    "DE_start: %ld\n"
    "DE_length: %ld\n",
    vcb->magic_signature,
    vcb->volume_size,
    vcb->block_size,
    vcb->num_blocks,
    vcb->FAT_start,
    vcb->FAT_length,
    vcb->DE_start,
    vcb->DE_length
  );
#endif
}