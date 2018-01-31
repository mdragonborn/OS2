#ifndef _PMTSLAB_H_
#define _PMT_SLAB_H_

#include "vm_declarations.h"

class PMTSlabAlloc {
public:
	static PhysicalAddress pmtSpace;
	static PageNum pmtSpaceSize;
	static int * freeBit;
	static void initSlab(PhysicalAddress address, PageNum num);
	static PhysicalAddress allocate();
	static void free(PhysicalAddress address);
private:
	PMTSlabAlloc() {};
};

#endif