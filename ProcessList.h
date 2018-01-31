
#ifndef _PROCLIST_H_
#define _PROCLIST_H_

#include "PMTable.h"
#include <map>

class Process;

class PMTSet
{
private:
	std::map<ProcessId,PMTable*> map;
public:
	bool insert(Process * proc, PMTable * table);
	void remove(int procId);
	PMTable * get(int procId);

};

#endif