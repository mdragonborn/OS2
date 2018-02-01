#ifndef _VMPGSLAB_H_
#define _VMPGSLAB_H_

#include "KernelProcess.h"
#include "KernelSystem.h"

#define PAGE_NUM(x) ((long)address - (long)vmSpace) / PAGE_SIZE
#define START_ADDR(x) (PhysicalAddress)((long)vmSpace+x*PAGE_SIZE)

class VMPGSlabAllocator {
public:
	static void initSlab(PhysicalAddress address, PageNum num, KernelSystem * sys);
	static void finalize();
	static PhysicalAddress allocate(KernelProcess * proc, VirtualAddress vadr);
	static void free(PhysicalAddress address);
	static int freeCount();
	static void access(PhysicalAddress address);
	static void tick();
	static void removeVictim(KernelProcess * proc);
private:
	static PhysicalAddress vmSpace;
	static PageNum vmSpaceSize;
	static char * refBits;
	static VirtualAddress * vmMapping;
	static KernelProcess ** rgProcess;
	static KernelSystem * pSystem;
	static int count;
	VMPGSlabAllocator() {};
};

#endif