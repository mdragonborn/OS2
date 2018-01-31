
#ifndef _PMTABLE_H_
#define _PMTABLE_H_

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

#include "vm_declarations.h"
#include "PMTSlabAlloc.h"
#include "Process.h"

struct PMT2Desc {
	PhysicalAddress physicalAddr=nullptr; //4B
	AccessType rwx; //1B
	bool free = true; //1B
	int cluster; //4B
	char padding[6];  //6B
};

class PMTable 
{
public:
	PhysicalAddress access(VirtualAddress address, AccessType access);
	Status announce(VirtualAddress address, PageNum pages, AccessType acc);
	Status assign(VirtualAddress address, PageNum pages, AccessType acc, void * content);
	Status remove(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	Status getPage(VirtualAddress address, int * buffer);
	Status returnCluster(VirtualAddress address, char * buffer);
	Status swapPage(VirtualAddress address, char * buffer);
	PMTable();
	~PMTable();
private:
	void initPMT2(int pmt1entry) {
		pmt1[pmt1entry] = (PMT2*)(PMTSlabAlloc::allocate());
		for (int i = 0; i < 64; i++)
		{
			pmt1[pmt1entry]->pmt2[i].physicalAddr = nullptr;
			pmt1[pmt1entry]->pmt2[i].free = true;
		}
	}

	bool checkPMT2HasContents(VirtualAddress address)
	{
		for (int i = 0; i < 64; i++)
			if (!pmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].free)
				return false;
		return true;
	}
	struct PMT2 {
		PMT2Desc pmt2[64]; //64 * 16B
	};
	PMT2 ** pmt1; //256 * 4B
	Process * pProcess;
};

#endif