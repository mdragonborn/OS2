
#include "Process.h"
#include "KernelProcess.h"
#include "KernelSystem.h"
#include "PMTSlabAlloc.h"
#include "VMPageSlabAllocator.h"
#include <iostream>
#include <assert.h>
#include "part.h"

KernelProcess::KernelProcess(ProcessId pid, Process * process)
{
	pProcess = process;
	PID = pid;
	rgPmt1 = (PMT2**)(PMTSlabAlloc::allocate());
};

KernelProcess::~KernelProcess()
{
	for (int i = 0; i < 256; i++)
		if (rgPmt1[i]) {
			PMTSlabAlloc::free(rgPmt1[i]);
			rgPmt1[i] = nullptr;
		}
	PMTSlabAlloc::free(rgPmt1);
};

ProcessId KernelProcess::getProcessId() const
{
	return PID;
};

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize,
	AccessType flags)
{

	int level2tables = (segmentSize / 64) + ((segmentSize % 64) ? 1 : 0);
	int left = segmentSize;
	for (int i = 0; i < segmentSize; i++, startAddress += PAGE_SIZE)
	{
		int pmt1entry = PMT1ENTRY(startAddress);
		int pmt2entry = PMT2ENTRY(startAddress);

		if (rgPmt1[pmt1entry] == nullptr){
			rgPmt1[pmt1entry] = (PMT2*)PMTSlabAlloc::allocate();
		initPMT2(pmt1entry);
		}
		
		PhysicalAddress vmSpace = VMPGSlabAllocator::allocate(this, startAddress);

		if (vmSpace == nullptr)
		{
			rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr = nullptr;
			int cluster = pSystem->writeToCluster(nullptr);
			assert(cluster != -1);
			rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster = cluster;
		}
		else
		{
			rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr = vmSpace;
			rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster = -1;
		}

		assert(!rgPmt1[pmt1entry]->pmt2[pmt2entry].allocated
			|| !rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster != -1);

		rgPmt1[pmt1entry]->pmt2[pmt2entry].free = false;
		rgPmt1[pmt1entry]->pmt2[pmt2entry].allocated = true;

		rgPmt1[pmt1entry]->pmt2[pmt2entry].rwx(flags);

	}	
	return OK;
};

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize,
	AccessType flags, void* content)
{
	unsigned long cntent = (unsigned long)content;
	createSegment(startAddress, segmentSize, flags);

	for (int i = 0; i < segmentSize; i++, startAddress += PAGE_SIZE, cntent += PAGE_SIZE)
	{
		int pmt1entry = PMT1ENTRY(startAddress);
		int pmt2entry = PMT2ENTRY(startAddress);

		if (rgPmt1[pmt1entry]->pmt2)
		{
			if(rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr != nullptr)
				memcpy(rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr, (void*)cntent, PAGE_SIZE);
			else
			{
			//	int cluster = pSystem->writeToCluster((void*)cntent);
			//	assert(cluster != -1);
			//	rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster = cluster;
				assert(pSystem->pPartition->writeCluster(rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster, (const char*)cntent));
			}
		}
		else 
			return TRAP;
	}
	return OK;
};

Status KernelProcess::deleteSegment(VirtualAddress startAddress)
{
	int pmt1entry = PMT1ENTRY(startAddress);
	int pmt2entry = PMT2ENTRY(startAddress);

	if (rgPmt1[pmt1entry]->pmt2)
	{
		VMPGSlabAllocator::free(rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr);
		rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr = nullptr;
		rgPmt1[pmt1entry]->pmt2[pmt2entry].free = true;

		if (!checkPMT2HasContents(startAddress))
		{
			PMTSlabAlloc::free((PhysicalAddress)(rgPmt1[pmt1entry]));
			rgPmt1[pmt1entry] = nullptr;
		}
		return OK;
	}
	else return TRAP;
};

Status KernelProcess::access(VirtualAddress address, AccessType flags)
{
	int pmt1entry = PMT1ENTRY(address);
	int pmt2entry = PMT2ENTRY(address);

	if (rgPmt1[pmt1entry]->pmt2)
	{
		AccessType rights = rgPmt1[pmt1entry]->pmt2[pmt2entry].rwx();
		if (rgPmt1[pmt1entry]->pmt2[pmt2entry].free == false && rgPmt1[pmt1entry]->pmt2[pmt2entry].allocated == true
			&& (rights == flags || (rights == READ_WRITE && (flags == READ || flags == WRITE))))
		{
			if (rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster == -1 && rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr != nullptr)
			{
				VMPGSlabAllocator::access(rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr);
			}
			else
			{
				return PAGE_FAULT;
			}
		}
		else 
			return TRAP;
	}
	else return TRAP;

	VMPGSlabAllocator::access(getPhysicalAddress(address));
	return OK;
}


Status KernelProcess::pageFault(VirtualAddress address)
{
	int clusterNumber;
	Status outcome;
	int result;
	int pmt1entry = PMT1ENTRY(address);
	int pmt2entry = PMT2ENTRY(address);
	if (rgPmt1[pmt1entry]->pmt2 == 0) return TRAP;
	if (!rgPmt1[pmt1entry]->pmt2[pmt2entry].allocated) return TRAP;
	VMPGSlabAllocator::removeVictim(this);
	char * buffer = (char*)VMPGSlabAllocator::allocate(this, address);
	if( pSystem->pPartition->readCluster(rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster, 
		buffer) ==0 )
		return TRAP;
		pSystem->diskSlots[rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster]=0;
		rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr = buffer;
		rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster = -1;
	
	return OK;
};
PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{
	int pmt1entry = PMT1ENTRY(address);
	int pmt2entry = PMT2ENTRY(address);

	if (rgPmt1[pmt1entry])
		if (rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr)
			return (void*)((long)(rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr)+(address&OFFSMASK));
	return nullptr;
};

void KernelProcess::initPMT2Entry(int pmt1entry, int pmt2entry) {
	rgPmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr = nullptr;
	rgPmt1[pmt1entry]->pmt2[pmt2entry].shmemAddr = nullptr;
	rgPmt1[pmt1entry]->pmt2[pmt2entry].free = true;
	rgPmt1[pmt1entry]->pmt2[pmt2entry].allocated = false;
	rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster = -1;
}



void KernelProcess::initPMT2(int pmt1entry)
{
	for (int i = 0; i < 64; i++)
	{
		rgPmt1[pmt1entry]->pmt2[i].physicalAddr = nullptr;
		rgPmt1[pmt1entry]->pmt2[i].shmemAddr = nullptr;
		rgPmt1[pmt1entry]->pmt2[i].free = true;
		rgPmt1[pmt1entry]->pmt2[i].allocated = false;
		rgPmt1[pmt1entry]->pmt2[i].cluster = -1;
	}
}

bool KernelProcess::checkPMT2HasContents(VirtualAddress address)
{
	for (int i = 0; i < 64; i++)
		if (!rgPmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].free)
			return false;
	return true;
}

void KernelProcess::setCluster(int cluster, VirtualAddress address)
{
	assert(rgPmt1[PMT1ENTRY(address)]);
	rgPmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].cluster = cluster;
	rgPmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].physicalAddr = 0;
}

#ifdef SHMEM

Process* KernelProcess::clone(ProcessId pid) 
{

}

Status KernelProcess::createSharedSegment(VirtualAddress startAddress,PageNum segmentSize, 
	const char* name, AccessType flags) 
{
	
}

Status KernelProcess::disconnectSharedSegment(const char* name) 
{

}

Status KernelProcess::deleteSharedSegment(const char* name) 
{

}

#endif