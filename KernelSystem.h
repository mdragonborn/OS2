#ifndef _KERNSYS_H_
#define _KERNSYS_H_

#include "vm_declarations.h"
#include <map>
#include <set>

class Process;
class ProcSet;
class Partition;
class KernelProcess;

#ifdef SHMEM
struct ShmemDescriptor
{
	PageNum size;
	std::set<KernelProcess *> pContainingProcess;
};
#endif

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
	ShmemDescriptor* findSharedSegment(const char* name);
	void addSharedSegment(const char * name, ShmemDescriptor* desc);
	int getRefCount(const char* name);
	void removeSharedSegment(const char* name);
	std::map<const char*, ShmemDescriptor*> shmemMap; //TODO
	void addShmem(ShmemDescriptor);
#endif
	KernelSystem *pSystem;
	Partition * pPartition;
	int * rgDiskSlots;
	ProcSet * rgProcSet;
	static int pidGenerator;
	friend class Process;
	friend class KernelProcess;
};



#endif //_KERNSYS_H_