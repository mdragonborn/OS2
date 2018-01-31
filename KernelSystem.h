#ifndef _KERNSYS_H_
#define _KERNSYS_H_

#include "vm_declarations.h"
#include "part.h"
#include "Process.cpp"
#include "ProcessList.h"

class Process;
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
private:

	KernelSystem *pSystem;
	Partition * pPartition;
	PMTSet pmts;

	static int pidGenerator;
	friend class Process;
	friend class KernelProcess;
};

#endif