#ifndef _SYSTEM_H_
#define _SYSTEM_H_

// File: System.h
#include "vm_declarations.h"
class Partition;
class Process;
class KernelProcess;
class KernelSystem;
class System {
public:
	System(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
		PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
		Partition* partition);
	~System();
	Process* createProcess();
	Time periodicJob();
	// Hardware job
	Status access(ProcessId pid, VirtualAddress address, AccessType type);
#ifdef SHMEM
	Process* cloneProcess(ProcessId pid);
#endif
private:
	KernelSystem *pSystem;
	friend class Process;
	friend class KernelProcess;
};

#endif