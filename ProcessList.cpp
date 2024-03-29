
#include "ProcessList.h"
#include "Process.h"
#include <map>

bool ProcSet::insert(Process * proc)
{
	if (rgProcMap.find(proc->getProcessId()) == rgProcMap.end())
	{
		rgProcMap.insert(std::make_pair(proc->getProcessId(), proc));
		return true;
	}
	else
		return false;
}

void ProcSet::remove(int procId) {
	rgProcMap.erase(procId);
}

Process * ProcSet::get(int procId)
{
	auto iter = rgProcMap.find(procId);
	if (iter != rgProcMap.end())
		return iter->second;
	else
		return nullptr;
}
