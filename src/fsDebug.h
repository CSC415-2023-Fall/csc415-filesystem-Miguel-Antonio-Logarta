#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef RELEASE
#define DEBUG
#endif

#include "fsLow.h"

// Debug statements that will only compile when the debug flag is on
void d_printVCB(VCB* vcb);

#endif
