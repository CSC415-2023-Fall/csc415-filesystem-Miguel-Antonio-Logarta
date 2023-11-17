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