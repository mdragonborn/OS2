
#include "Process.h"
#include "KernelProcess.h"
#include "KernelSystem.h"

KernelProcess::KernelProcess(ProcessId pid, Process * process)
{
	pProcess = process;
	PID = pid;
};
KernelProcess::~KernelProcess()
{
	delete pmt;
};
ProcessId KernelProcess::getProcessId() const
{
	return PID;
};
Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize,
	AccessType flags)
{
	return pmt->announce(startAddress, segmentSize, flags);
};
Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize,
	AccessType flags, void* content)
{
	return pmt->assign(startAddress, segmentSize, flags, content);
};
Status KernelProcess::deleteSegment(VirtualAddress startAddress)
{
	return pmt->remove(startAddress);
};
Status KernelProcess::pageFault(VirtualAddress address)
{
	char * buffer; int clusterNumber;
	Status outcome;
	if ((outcome = pmt->getPage(address, &clusterNumber)) != OK)
		return outcome;
	pSystem->pPartition->readCluster(clusterNumber, buffer);
	pmt->returnCluster(address, buffer);

	return OK;
};
PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{
	return pmt->getPhysicalAddress(address);
};