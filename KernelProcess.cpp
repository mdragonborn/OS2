
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
#ifdef SHMEM
		assert(rgPmt1[pmt1entry]->pmt2[pmt2entry].shmemAddr != 0);
#endif
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
#ifdef SHMEM
			else if (rgPmt1[pmt1entry]->pmt2[pmt2entry].shmemAddr != nullptr)
			{
				VMPGSlabAllocator::access(rgPmt1[pmt1entry]->pmt2[pmt2entry].shmemAddr);
			}
#endif
			else
			{
				return PAGE_FAULT;
			}
		}
		else 
			return TRAP;
	}
	else return TRAP;

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
		pSystem->rgDiskSlots[rgPmt1[pmt1entry]->pmt2[pmt2entry].cluster]=0;
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
	Process * proc = pSystem->createProcess();
	KernelProcess * kpr = proc->pProcess;
	kpr->rgPmt1 = (PMT2**)PMTSlabAlloc::allocate();
	for (int i = 0; i < 256; i++)
	{
		if (rgPmt1[i] != nullptr)
		{
			kpr->rgPmt1[i] = (PMT2*)PMTSlabAlloc::allocate();
			kpr->initPMT2(i);
			for (int j = 0; j < 64; j++)
			{
				kpr->rgPmt1[i]->pmt2[j].accessBits = rgPmt1[i]->pmt2[j].accessBits;
				kpr->rgPmt1[i]->pmt2[j].allocated = rgPmt1[i]->pmt2[j].accessBits;
				kpr->rgPmt1[i]->pmt2[j].cluster = rgPmt1[i]->pmt2[j].cluster;
				kpr->rgPmt1[i]->pmt2[j].free = rgPmt1[i]->pmt2[j].free;
				if (rgPmt1[i]->pmt2[j].physicalAddr)
				{
					kpr->rgPmt1[i]->pmt2[j].physicalAddr = VMPGSlabAllocator::allocate(kpr, i << 16 || j << 10);
				}
				else if (rgPmt1[i]->pmt2[j].shmemAddr)
				{
					kpr->rgPmt1[i]->pmt2[j].physicalAddr = rgPmt1[i]->pmt2[j].shmemAddr;
				}
			}
		}
	}
	for (auto iter = usedShmem.begin(); iter != usedShmem.end(); iter++)
	{
		kpr->usedShmem.insert(std::make_pair(iter->first, iter->second));
		pSystem->findSharedSegment(iter->first)->pContainingProcess.insert(kpr);
	}

	return proc;
}


Status KernelProcess::createSharedSegment(VirtualAddress startAddress,PageNum segmentSize, 
	const char* name, AccessType flags) 
{
	ShmemDescriptor* original = pSystem->findSharedSegment(name);
	if (original != nullptr)
	{
		createSegment(startAddress, segmentSize, flags);
		KernelProcess * proc = *(original->pContainingProcess.begin());
		VirtualAddress contStartAddress = proc->getShmemStart(name);

		for (int i = 0; i < segmentSize; i++)
		{
			int pmt1ent = PMT1ENTRY(startAddress + i);
			int pmt2ent = PMT2ENTRY(startAddress + i);
			PhysicalAddress pa = proc->getPhysicalAddress(contStartAddress + i);
			assert(pa);
			VMPGSlabAllocator::free(rgPmt1[pmt1ent]->pmt2[pmt2ent].physicalAddr);
			rgPmt1[pmt1ent]->pmt2[pmt2ent].physicalAddr = nullptr;
			rgPmt1[pmt1ent]->pmt2[pmt2ent].shmemAddr = pa;
		}
		original->pContainingProcess.insert(this);
		usedShmem.insert(std::make_pair(name, startAddress));
	}
	else
	{
		for (int i = 0; i < segmentSize; i++)
		{
			int pmt1ent = PMT1ENTRY(startAddress + i);
			int pmt2ent = PMT2ENTRY(startAddress + i);
			rgPmt1[pmt1ent]->pmt2[pmt2ent].shmemAddr = rgPmt1[pmt1ent]->pmt2[pmt2ent].physicalAddr;
			rgPmt1[pmt1ent]->pmt2[pmt2ent].physicalAddr = nullptr;
		}

		ShmemDescriptor * desc = new ShmemDescriptor;
		desc->size = segmentSize;
		desc->pContainingProcess.insert(this);
		pSystem->addSharedSegment(name, desc);

	}
	usedShmem.insert(std::make_pair(name, startAddress));
	return OK;
}

Status KernelProcess::disconnectSharedSegment(const char* name, bool del) 
{
	ShmemDescriptor * desc = pSystem->findSharedSegment(name);
	VirtualAddress startAddress = getShmemStart(name);
	assert(startAddress);
	assert(desc);
	for (int i = 0; i < desc->size; i++)
	{
		int pmt1ent = PMT1ENTRY(startAddress + i);
		int pmt2ent = PMT2ENTRY(startAddress + i);
		if (del)
			VMPGSlabAllocator::free(rgPmt1[pmt1ent]->pmt2[pmt2ent].shmemAddr);

		rgPmt1[pmt1ent]->pmt2[pmt1ent].shmemAddr = 0;
		rgPmt1[pmt1ent]->pmt2[pmt1ent].allocated = false;
		rgPmt1[pmt1ent]->pmt2[pmt1ent].free = true;

		if (!checkPMT2HasContents(startAddress + i))
			PMTSlabAlloc::free(rgPmt1[pmt1ent]->pmt2);
	}
	return OK;
}

Status KernelProcess::deleteSharedSegment(const char* name) 
{
	ShmemDescriptor * desc = pSystem->findSharedSegment(name);
	int i=0;
	for (auto iter = desc->pContainingProcess.begin(); i < desc->pContainingProcess.size() - 1; iter++, i++)
		(*iter)->disconnectSharedSegment(name);
	auto iter = --desc->pContainingProcess.end();
	(*iter)->disconnectSharedSegment(name, true);
	pSystem->removeSharedSegment(name);

	return OK;
}

VirtualAddress KernelProcess::getShmemStart(const char * name)
{
	auto find = usedShmem.find(name);
	if (find != usedShmem.end())
		return find->second;
	else
		return 0;
}


#endif