#include <stdlib.h>
#include <string.h>

#include "partition.h"
#include "mfs.h"
#include "fsLow.h"
#include "debug.h"

// Initialize global variables to NULL
VCB* g_vcb = NULL;
FAT_block* g_FAT = NULL;
// fdFAT* g_FAT = NULL;

VCB *fs_getvcb() {
  // Returns Volume Control Block and caches the result to g_vcb

  // If g_vcb is not initialized, read vcb from the disk and cache it
  if (g_vcb == NULL) {
    g_vcb = fs_malloc(sizeof(VCB), "Unable to malloc VCB");
    unsigned char *buffer = fs_malloc(MINBLOCKSIZE, "Unable to malloc read buffer");
    buffer = fs_LBAread(buffer, 1, 0, "Unable to read VCB from disk");
    memcpy(g_vcb, buffer, sizeof(VCB));
    
    free(buffer);
  }

  // Make a copy of g_vcb and return it
  VCB* vcbCopy = fs_malloc(sizeof(VCB), "Unable to malloc copy of VCB");
  memcpy(vcbCopy, g_vcb, sizeof(VCB));

  return vcbCopy;
}

VCB *fs_writevcb(VCB* vcb) {
  // Writes the Volume Control Block to disk and caches the new VCB to g_vcb

  if (g_vcb == NULL) {
    g_vcb = fs_malloc(sizeof(VCB), "Unable to malloc g_vcb");
  }

  // Replace g_vcb with new VCB
  /*
    Why make a copy?
      It's to handle undefined behavior when the 
      addresses of source (vcb) and destination (g_vcb)
      overlap.
  */
  VCB* vcbCopy = fs_malloc(sizeof(VCB), "Unable to malloc VCB");
  memcpy(vcbCopy, vcb, sizeof(VCB));
  memcpy(g_vcb, vcbCopy, sizeof(VCB));

  unsigned char* buffer = fs_malloc_buff(sizeof(VCB), MINBLOCKSIZE, "Unable to malloc buffer for VCB");
  memcpy(buffer, g_vcb, sizeof(VCB));
  fs_LBAwrite(buffer, 1, 0, "Unable to write VCB to disk");

  free(vcbCopy);
  free(buffer);

	return g_vcb;
}

void fs_freevcb(VCB* vcb) {
	free(vcb);
}

FAT_block *fs_getFAT() {
  // Returns the FAT table from the disk and caches result to g_FAT

  VCB* vcb = fs_getvcb();

  if (g_FAT == NULL) {
    debug_print("g fat nulll %ld, %ld\n", vcb->num_blocks, vcb->FAT_length);
    unsigned char* buffer = fs_malloc_buff(sizeof(FAT_block)*vcb->num_blocks, vcb->block_size, "Unable to malloc FAT buffer");
    fs_LBAread(buffer, vcb->FAT_length, vcb->FAT_start, "Unable to read FAT");
    g_FAT = fs_malloc(sizeof(FAT_block)*vcb->num_blocks, "Unable to malloc g_FAT");
    memcpy(g_FAT, buffer, sizeof(FAT_block)*vcb->num_blocks);
    free(buffer);
  }

  FAT_block *FATCopy = fs_malloc(sizeof(FAT_block)*vcb->num_blocks, "Unable to malloc FATCopy");
  memcpy(FATCopy, g_FAT, sizeof(FAT_block)*vcb->num_blocks);

  free(vcb);

	return FATCopy;
}

FAT_block *fs_writeFAT(FAT_block* fat, uint64_t numBlocks) {
	// Writes the FAT to disk and caches the new FAT to g_FAT

  if (g_FAT == NULL) {
    g_FAT = fs_malloc(sizeof(FAT_block)*numBlocks, "Unable to malloc g_FAT");
  }

  // Replace g_FAT with new VCB
  /*
    Why make a copy?
      It's to handle undefined behavior when the 
      addresses of source (fat) and destination (g_FAT)
      overlap.
  */
  FAT_block* FATCopy = fs_malloc(sizeof(FAT_block)*numBlocks, "Unable to malloc FAT");
  memcpy(FATCopy, g_FAT, sizeof(FAT_block)*numBlocks);
  memcpy(g_FAT, fat, sizeof(FAT_block)*numBlocks);

  unsigned char* buffer = fs_malloc_buff(sizeof(FAT_block)*numBlocks, MINBLOCKSIZE, "Unable to malloc buffer for FAT");
  memcpy(buffer, g_FAT, sizeof(FAT_block)*numBlocks);

  VCB* vcb = fs_getvcb();

  debug_print("FAt info: %ld %ld\n", vcb->FAT_length, vcb->FAT_start);
  fs_LBAwrite(buffer, vcb->FAT_length, vcb->FAT_start, "Unable to write FAT to disk");
  debug_print("Resultof after buffer: \n");

  free(FATCopy);
  free(buffer);
  fs_freevcb(vcb);

	return g_FAT;
}

void fs_freefat(FAT_block* fat) {
	free(fat);
}

fat_index fs_findFreeBlock(FAT_block* fat) {
  int numBlocks = fs_getFATLength();
  for (int i = 0; i < numBlocks; i++) {
    if (fat[i].in_use == 0) {
      return i;
    }
  }
  return -1;
}

int fs_getFATLength() {
  VCB* vcb = fs_getvcb();
  int length = vcb->FAT_length;
  fs_freevcb(vcb);
  return length;
}

lba_offset fs_getLBABlock(fat_index FATIndex){
  // Returns lba block that is mapped to the FATIndex
  VCB* vcb = fs_getvcb();
  int startPos = vcb->FAT_length + vcb->FAT_start;  // FATIndex 0, or the first block after the FAT table.
  fs_freevcb(vcb);
  return startPos + FATIndex;
}

fat_index fs_getFATIndex(lba_offset lbaOffset) {
  VCB* vcb = fs_getvcb();
  
  // Ex: Root dir is written on lba block 154 which is mapped to FATIndex 0
  // fatIndex = 154 - 153 - 1 = 0
  int fatIndex = lbaOffset - vcb->FAT_length - vcb->FAT_start;
  
  fs_freevcb(vcb);

  return fatIndex;
}