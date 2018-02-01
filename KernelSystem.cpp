
#include "KernelSystem.h"
#include "VMPageSlabAllocator.h"
#include "PMTSlabAlloc.h"
#include "ProcessList.h"
#include "Process.h"
#include <assert.h>
#include "part.h"

int KernelSystem::pidGenerator=1;

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
	PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
	Partition* partition)
{
	this->pPartition = partition;
	VMPGSlabAllocator::initSlab(processVMSpace, processVMSpaceSize, this);
	PMTSlabAlloc::initSlab(pmtSpace, pmtSpaceSize);
	rgProcSet = new ProcSet;
	diskSlots = new int[pPartition->getNumOfClusters()];
	for (unsigned int i = 0; i < pPartition->getNumOfClusters(); i++)
		diskSlots[i] = 0;
};

KernelSystem::~KernelSystem() 
{
	VMPGSlabAllocator::finalize();
	PMTSlabAlloc::finalize();
};

Process* KernelSystem::createProcess()
{
	Process * proc = new Process(pidGenerator++);
	proc->pProcess->pSystem = this;
	rgProcSet->insert(proc);

	return proc;
};

Time KernelSystem::periodicJob()
{
	VMPGSlabAllocator::tick();
	return 260;
};
// Hardware job
Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	Process * process = rgProcSet->get(pid);
	if (!process)
		return TRAP;
	return process->pProcess->access(address, type);
};

int KernelSystem::writeToCluster(PhysicalAddress address)
{
	unsigned int i;
	for (i = 0; i < pPartition->getNumOfClusters(); i++)
		if (diskSlots[i] == 0) break;
	if (pPartition->getNumOfClusters() == i) assert(0); //TODO
	if (address != nullptr)
	{
		if (pPartition->writeCluster(i, (const char *)address))
		{
			diskSlots[i] = 1;
			return i;
		}
		else
			return -1;
	}
	else
	{
		diskSlots[i] = 1;
		return i;
	}
}

int KernelSystem::readAndFreeCluster(unsigned int cluster, char * buffer)
{
	int outcome;
	assert(cluster > 0 && cluster < pPartition->getNumOfClusters());
	if ((outcome=pPartition->readCluster(cluster, (char *)buffer)) == 1)
		diskSlots[cluster] = 0;
	else assert(0);
	return outcome;
}

#ifdef SHMEM

Process* KernelSystem::cloneProcess(ProcessId pid)
{
	Process * proc = rgProcSet->get(pid);
	return proc->clone(pid);
	PhysicalAddress * findSharedSegment(const char* name);
	void createSharedSegment(const char* name);
	int getRefCount(const char* name);
}

PhysicalAddress KernelSystem::createSharedSegment(VirtualAddress startAddress, PageNum segmentSize,
	const char* name, AccessType flags)
{
	auto find = shmemMap.find(name);
	if (find == shmemMap.end())
	{
		
		for (int i = 0; i < segmentSize; i++)
		{
			PhysicalAddress segment = VMPGSlabAllocator::allocate((KernelProcess*)0xffffffff, startAddress);
			shmemMap.insert(std::make_pair(name, segment));
		}
		return segment;
	}
	else
		return nullptr;
}


PhysicalAddress KernelSystem::findSharedSegment(const char* name)
{
	auto find = shmemMap.find(name);
	if (find != shmemMap.end())
	{
		shmemCountMap[name]++;
		return find->second;
	}
	else return nullptr;
}

int KernelSystem::detachSharedSegment(const char* name)
{
	auto find = shmemMap.find(name);
	if (find != shmemMap.end())
	{
		shmemCountMap[name]--;
		if (shmemCountMap[name] == 0)
		{
			VMPGSlabAllocator::free(find->second);
			shmemMap.erase(name);
			shmemCountMap.erase(name);
			return 1;
		}
		else
			return 0;
	}
	else return -1;
}

int KernelSystem::getRefCount(const char* name)
{
	auto find = shmemCountMap.find(name);
	if (find != shmemCountMap.end())
	{
		return shmemCountMap[name];
	}
	else
		return 0;
}

#endif