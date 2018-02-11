
#include "vm_declarations.h"
#include "VMPageSlabAllocator.h"

PhysicalAddress VMPGSlabAllocator::vmSpace=0;
PageNum VMPGSlabAllocator::vmSpaceSize=0;
char * VMPGSlabAllocator::refBits=0;
VirtualAddress * VMPGSlabAllocator::vmMapping=0;
KernelProcess ** VMPGSlabAllocator::rgProcess=0;
KernelSystem * VMPGSlabAllocator::pSystem=0;
int VMPGSlabAllocator::count=0;

void VMPGSlabAllocator::initSlab(PhysicalAddress address, PageNum num, KernelSystem * sys)
{
	vmSpace = address;
	vmSpaceSize = num;
	count = num;
	assert(num > 0);
	assert(rgProcess==nullptr);
	assert(refBits == nullptr);
	refBits = new char[num];
	vmMapping = new VirtualAddress[num];
	rgProcess = new KernelProcess*[num];
	pSystem = sys;
	for (int i = 0; i < num; i++)
	{
		refBits[i] = 0;
		rgProcess[i] = nullptr;
		vmMapping[i] = 0;
	}
};

PhysicalAddress VMPGSlabAllocator::allocate(KernelProcess * proc, VirtualAddress vadr) 
{
	int i = 0;
	PhysicalAddress ret;
	//TODO clock handle
	for (i = 0; i < vmSpaceSize && rgProcess[i] != nullptr; i++);

	if(count!=0)
	{
		ret = (void*)((long)vmSpace + i * PAGE_SIZE);
		for (int j = 0; j < PAGE_SIZE / sizeof(long); j++)
			((long*)(ret))[j] = 0;
 		refBits[i] = 0x80;
		rgProcess[i] = proc;
		vmMapping[i] = vadr;
		count--;
	}
	else 
		ret = nullptr;

	return ret;
};

void VMPGSlabAllocator::tick() {
	for (int i = 0; i < vmSpaceSize; i++)
		refBits[i] >>= 1;
}

void VMPGSlabAllocator::access(PhysicalAddress address)
{
	refBits[PAGE_NUM(address)] &= 0x80;
}

void VMPGSlabAllocator::removeVictim(KernelProcess * proc)
{
	if (count > 0) return;

	int minimum = 0;
	for (int i = 0; i < vmSpaceSize; i++)
	{
		if (refBits[i] < refBits[minimum] && rgProcess[i]!=proc 
#ifdef SHMEM
			&& vmMapping[i]==0xffffffff
#endif
			)
			minimum = i;
	}
	//assert(rgProcess[minimum] != proc);

	int cluster = pSystem->writeToCluster(START_ADDR(minimum));

	rgProcess[minimum]->setCluster(cluster, vmMapping[minimum]);
	
	vmMapping[minimum] = 0;
	rgProcess[minimum] = nullptr;
	refBits[minimum] = 0;
	count++;
}

void VMPGSlabAllocator::free(PhysicalAddress address) 
{
	int pageNum = ((long)address - (long)vmSpace) / PAGE_SIZE;
	assert(pageNum >= 0 && pageNum < vmSpaceSize);
	rgProcess[pageNum] = nullptr;
	refBits[pageNum] = 0;
	count++;
};

int VMPGSlabAllocator::freeCount() 
{
	return count;
};

void VMPGSlabAllocator::finalize()
{
	delete rgProcess;
	delete refBits;
}