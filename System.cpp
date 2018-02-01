
#include "System.h"
#include "KernelSystem.h"

System::System::System(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
	PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
	Partition* partition)
{
	pSystem = new KernelSystem(processVMSpace, processVMSpaceSize, pmtSpace, pmtSpaceSize, partition);
};
System::~System() 
{
	delete pSystem;
};
Process* System::createProcess()
{
	return pSystem->createProcess();
};
Time System::periodicJob()
{
	return pSystem->periodicJob();
};
// Hardware job
Status System::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	return pSystem->access(pid, address, type);
};

#ifdef SHMEM
Process* System::cloneProcess(ProcessId pid)
{
	return pSystem->cloneProcess(pid);
}
#endif
