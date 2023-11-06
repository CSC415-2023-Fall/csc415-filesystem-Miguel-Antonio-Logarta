<<<<<<< HEAD
=======
#ifndef _DEBUG_H
#define _DEBUG_H

#include "fsLow.h"

>>>>>>> origin/Miguel
#define DEBUG 1

// Debug statements that will only compile when the debug flag is on
#ifdef DEBUG
<<<<<<< HEAD
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif
=======
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

void printVCB(VCB* vcb) {
	printf("Printing VCB\nmagic_signature: %ld\nvolume_size: %ld\nblock_size: %ld\nnum_blocks: %ld\nFAT_start: %ld\nFAT_length: %ld\nDE_start: %ld\nDE_length: %ld\n", 
	vcb->magic_signature,
  vcb->volume_size,
  vcb->block_size,
  vcb->num_blocks,
  vcb->FAT_start,
  vcb->FAT_length,
  vcb->DE_start,
  vcb->DE_length);
}

#endif
>>>>>>> origin/Miguel
