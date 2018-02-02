
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
	rgDiskSlots = new int[pPartition->getNumOfClusters()];
	for (unsigned int i = 0; i < pPartition->getNumOfClusters(); i++)
		rgDiskSlots[i] = 0;
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
		if (rgDiskSlots[i] == 0) break;
	if (pPartition->getNumOfClusters() == i) assert(0); //TODO
	if (address != nullptr)
	{
		if (pPartition->writeCluster(i, (const char *)address))
		{
			rgDiskSlots[i] = 1;
			return i;
		}
		else
			return -1;
	}
	else
	{
		rgDiskSlots[i] = 1;
		return i;
	}
}

int KernelSystem::readAndFreeCluster(unsigned int cluster, char * buffer)
{
	int outcome;
	assert(cluster > 0 && cluster < pPartition->getNumOfClusters());
	if ((outcome=pPartition->readCluster(cluster, (char *)buffer)) == 1)
		rgDiskSlots[cluster] = 0;
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

void KernelSystem::addSharedSegment(const char* name, ShmemDescriptor* desc)
{
	assert(shmemMap.find(name) == shmemMap.end());
	shmemMap.insert(std::make_pair(name, desc));
}

ShmemDescriptor* KernelSystem::findSharedSegment(const char* name)
{
	auto find = shmemMap.find(name);
	if (find != shmemMap.end())
	{
		return find->second;
	}
	else return nullptr;
}

int KernelSystem::getRefCount(const char* name)
{
	auto find = shmemMap.find(name);
	if (find != shmemMap.end())
	{
		return (shmemMap[name])->pContainingProcess.size();
	}
	else
		return 0;
}

void KernelSystem::removeSharedSegment(const char * name)
{
	shmemMap.erase(name);
}

#endif