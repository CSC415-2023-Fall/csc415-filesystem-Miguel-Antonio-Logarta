#include <stdlib.h>
#include <string.h>

#include "partition.h"
#include "mfs.h"
#include "fsLow.h"

VCB *fs_getvcb() {
  // Returns Volume Control Block and caches the result to g_vcb
	if (g_vcb != NULL) {
		return g_vcb;
	}

	// Allocate the VCB
	g_vcb = fs_malloc(sizeof(VCB), "Unable to malloc VCB");
  unsigned char *buffer = fs_LBAread(sizeof(VCB), MINBLOCKSIZE, 1, 0, "Unable to read VCB from disk");
  memcpy(g_vcb, buffer, sizeof(VCB));
	
  free(buffer);

  return g_vcb;
}

VCB *fs_writevcb(VCB* vcb) {
  // Writes the Volume Control Block to disk and caches the new VCB to g_vcb

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
	return NULL;
}

FAT_block *fs_writeFAT(FAT_block* fat) {
	return NULL;
}

void fs_freefat(FAT_block* fat) {
	free(fat);
}