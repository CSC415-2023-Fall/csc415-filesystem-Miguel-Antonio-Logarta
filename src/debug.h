#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef RELEASE
#define DEBUG
#endif

#include "partition.h"

// Debug functions that will only do something when the debug flag is on
#ifdef DEBUG
#define debug_print(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define debug_print(fmt, ...) do {} while (0)
#endif

void d_printVCB(VCB* vcb);

#endif
