
#include "KernelSystem.h"

int KernelSystem::pidGenerator=1;

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
	PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
	Partition* partition)
{
	this->pPartition = partition;

};

KernelSystem::~KernelSystem() 
{

};

Process* KernelSystem::createProcess()
{
	Process * proc = new Process(pidGenerator++);
	PMTable * table = new PMTable();
	proc->pProcess->pSystem = this;
	pmts.insert(proc, table);
	return proc;
};

Time KernelSystem::periodicJob()
{
	//tick
};
// Hardware job
Status access(ProcessId pid, VirtualAddress address, AccessType type)
{

};