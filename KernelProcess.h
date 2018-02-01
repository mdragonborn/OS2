#ifndef _KERNPROC_H_
#define _KERNPROC_H_

#define RBIT 0b100
#define WBIT 0b010
#define XBIT 0b001

#define PMT1MASK 0xFF0000
#define PMT2MASK 0x00FC00
#define OFFSMASK 0x0003FF

#define PMT1SH 16
#define PMT2SH 10

#define PMT1ENTRY(x) ((x & PMT1MASK) >> PMT1SH)
#define PMT2ENTRY(x) ((x & PMT2MASK) >> PMT2SH)
#define OFFSET(x) (x & OFFSMASK)


// File: Process.h
#include "vm_declarations.h"

class System;
class KernelSystem;
class Process;

struct PMT2Desc {
	PhysicalAddress physicalAddr = nullptr; //4B
	AccessType rwx; //1B
	bool free = true; //1B
	bool allocated = false; //1B
	int cluster; //4B
};

struct PMT2 {
	PMT2Desc pmt2[64]; //64 * 16B
};


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
	Status access(VirtualAddress address, AccessType flags);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
private:
	Process *pProcess;
	KernelSystem * pSystem;
	PMT2 ** rgPmt1; //256 * 4B
	int PID;
	friend class System;
	friend class KernelSystem;
	friend class VMPGSlabAllocator;

	void initPMT2Entry(int pmt1entry, int pmt2entry);
	void initPMT2(int pmt1entry);
	bool checkPMT2HasContents(VirtualAddress address);
	
	void setCluster(int cluster, VirtualAddress address);

};


#endif