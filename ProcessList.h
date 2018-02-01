#pragma once

#include <map>
class Process;
typedef unsigned ProcessId;

class ProcSet {
private:
	std::map<const ProcessId,Process*> rgProcMap;
public:
	bool insert(Process * proc);
	void remove(int procId);
	Process * get(int procId);
};