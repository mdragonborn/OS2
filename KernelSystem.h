#ifndef _KERNSYS_H_
#define _KERNSYS_H_

#include "vm_declarations.h"
#include <map>

class Process;
class ProcSet;
class Partition;

class KernelSystem {
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
		PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
		Partition* partition);
	~KernelSystem();
	Process* createProcess();
	Time periodicJob();
	// Hardware job
	Status access(ProcessId pid, VirtualAddress address, AccessType type);
	int writeToCluster(PhysicalAddress address);
	int readAndFreeCluster(unsigned int cluster, char * buffer);
#ifdef SHMEM
	Process* cloneProcess(ProcessId pid);
#endif

private:
#ifdef SHMEM
	PhysicalAddress findSharedSegment(const char* name);
	PhysicalAddress createSharedSegment(VirtualAddress startAddress, PageNum segmentSize,
		const char* name, AccessType flags);
	int getRefCount(const char* name);
	int detachSharedSegment(const char* name);
	std::map<const char*, PhysicalAddress> shmemMap; //TODO
	std::map<const char*, int> shmemCountMap; //TODO

#endif
	KernelSystem *pSystem;
	Partition * pPartition;
	int * diskSlots;
	ProcSet * rgProcSet;
	static int pidGenerator;
	friend class Process;
	friend class KernelProcess;
};

#endif