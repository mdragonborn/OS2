#ifndef _KERNPROC_H_
#define _KERNPROC_H_

// File: Process.h
#include "vm_declarations.h"
#include "PMTable.h"

class System;
class KernelSystem;
class Process;

class KernelProcess {
public:
	KernelProcess(ProcessId pid, Process * process);
	~KernelProcess();
	ProcessId getProcessId() const;
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessType flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessType flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);
	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
private:
	Process *pProcess;
	KernelSystem * pSystem;
	PMTable * pmt;
	int PID;
	friend class System;
	friend class KernelSystem;
};

#endif