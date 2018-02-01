#ifndef _VMDEC_H_
#define _VMDEC_H_

#include "assert.h"

#define SHMEM

// File: vm_declarations.h
typedef unsigned long PageNum;
typedef unsigned long VirtualAddress;
typedef void* PhysicalAddress;
typedef unsigned long Time;
enum Status { OK, PAGE_FAULT, TRAP };
enum AccessType { READ, WRITE, READ_WRITE, EXECUTE };
typedef unsigned ProcessId;
#define PAGE_SIZE 1024

#endif