/**************************************************************
* Class:  CSC-415-02 Fall 2023
* Names: Mohammed Deeb, Miguel Antonio Logarta, Abdarrahman Ayyaz, Avitaj Singh Bhangoo
* Student IDs: 922875899, 923062683, 922859909, 921303783
* GitHub Name:
* Group Name: Byte Builders
* Project: Basic File System
*
* File: debug.h
*
* Description: Contains debugging tools to help diagnose issues with the file system
*
**************************************************************/

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
