
#include "ProcessList.h"
#include "Process.h"

bool PMTSet::insert(Process * proc, PMTable * table)
{
	if (map.find(proc->getProcessId()) == map.end())
	{
		map.insert(std::make_pair(proc->getProcessId(), table));
		return true;
	}
	else
		return false;
}

void PMTSet::remove(int procId) {
	map.erase(procId);
}

PMTable * PMTSet::get(int procId)
{
	return (*(map.find(procId))).second;
}
