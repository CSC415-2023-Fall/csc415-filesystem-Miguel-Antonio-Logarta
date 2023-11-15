#include <sys/types.h>
#include <stdio.h>
#include "fsDebug.h"

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

void d_printVCB(VCB* vcb) {
#ifdef DEBUG
	printf("Printing VCB\nmagic_signature: %ld\nvolume_size: %ld\nblock_size: %ld\nnum_blocks: %ld\nFAT_start: %ld\nFAT_length: %ld\nDE_start: %ld\nDE_length: %ld\n", 
	vcb->magic_signature,
  vcb->volume_size,
  vcb->block_size,
  vcb->num_blocks,
  vcb->FAT_start,
  vcb->FAT_length,
  vcb->DE_start,
  vcb->DE_length);
#endif
}