#ifndef _KERNSYS_H_
#define _KERNSYS_H_

#include "vm_declarations.h"


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
	int readAndFreeCluster(unsigned int cluster, PhysicalAddress * buffer);
private:

	KernelSystem *pSystem;
	Partition * pPartition;
	int * diskSlots;
	ProcSet * rgProcSet;
	static int pidGenerator;
	friend class Process;
	friend class KernelProcess;
};

#endif