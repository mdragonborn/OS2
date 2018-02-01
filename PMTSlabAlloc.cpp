
#include "PMTSlabAlloc.h"
#include "KernelProcess.h"
PhysicalAddress PMTSlabAlloc::pmtSpace=0;
PageNum PMTSlabAlloc::pmtSpaceSize=0;
int * PMTSlabAlloc::freeBit=0;

void PMTSlabAlloc::initSlab(PhysicalAddress address, PageNum num)
{
	pmtSpace = address;
	pmtSpaceSize = num;
	assert(num > 0);
	assert(freeBit==nullptr);
	freeBit = new int[num];
	for (int i = 0; i < num; i++) freeBit[i] = 0;
	
};
PhysicalAddress PMTSlabAlloc::allocate()
{
	int i = 0;
	PhysicalAddress ret;
	for (i = 0; i < pmtSpaceSize; i++)
		if (freeBit[i] == 0) break;
	if (i != pmtSpaceSize)
	{
		ret =(PMT2*)pmtSpace+i;
		for (int i = 0; i < PAGE_SIZE; i++)
			((int*)(ret))[i] = 0;
		freeBit[i] = 1;
	}
	else ret = nullptr;
	
	return ret;

};
void PMTSlabAlloc::free(PhysicalAddress address)
{
	int pageNum = ((long)address - (long)pmtSpace)/ PAGE_SIZE;
	assert(pageNum > 0 && pageNum < pmtSpaceSize);
	freeBit[pageNum] = 0;
};

void PMTSlabAlloc::finalize()
{
	delete freeBit;
}