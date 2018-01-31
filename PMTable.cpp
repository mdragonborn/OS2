
#include "PMTable.h"

//VM 8+6+10b

PhysicalAddress PMTable::access(VirtualAddress address, AccessType access)
{
	int pmt1entry = PMT1ENTRY(address);
	int pmt2entry = PMT2ENTRY(address);
	long offset = OFFSET(address);

	assert(pmt1entry > 0 && pmt1entry < 256);
	assert(pmt2entry > 0 && pmt2entry < 64);

	if (!pmt1[pmt1entry])
		return nullptr;

	if (pmt1[pmt1entry]->pmt2[pmt2entry].free)
		return nullptr;

	AccessType allowed= pmt1[pmt1entry]->pmt2[pmt2entry].rwx;

	if (allowed == access ||
		(allowed == READ || allowed == WRITE && allowed == READ_WRITE))
	{
		if (pmt1[pmt1entry]->pmt2[pmt2entry].cluster == 0)
			pProcess->pageFault(address);

		return (void*)((long)(pmt1[pmt1entry]->pmt2[pmt2entry].physicalAddr) + offset);
	}
	else
		return nullptr;
};

Status PMTable::announce(VirtualAddress address, PageNum pages, AccessType acc)
{
	if (access(address, acc) != nullptr)
	{
		for (int i = 0; i < pages; i++, address+=PAGE_SIZE)
		{
			int pmt1entry = PMT1ENTRY(address);
			int pmt2entry = PMT2ENTRY(address);

			if (pmt1[pmt1entry] == nullptr)
			{
				initPMT2(pmt1entry);
				pmt1[pmt1entry]->pmt2[pmt2entry].free = false;
				pmt1[pmt1entry]->pmt2[pmt2entry].rwx = acc;
			}
			else
			{
				if (pmt1[pmt1entry]->pmt2[pmt2entry].free == true)
				{
					pmt1[pmt1entry]->pmt2[pmt2entry].free = false;
					pmt1[pmt1entry]->pmt2[pmt2entry].rwx = acc;
				}
			}
		}
	}
	return OK;
};

Status PMTable::assign(VirtualAddress address, PageNum pages, AccessType acc, void * content)
{
	unsigned long cntent = (unsigned long)content;
	for (int i = 0; i < pages; i++, address+=PAGE_SIZE, cntent+=PAGE_SIZE)
	{
		if (pmt1[PMT1ENTRY(address)]->pmt2)
		{
			if ((pmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].free == false))			{
				pmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].physicalAddr = (void *)cntent;
			}
		}
		else if (pmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].physicalAddr == nullptr)
		{
			return PAGE_FAULT;
		}
		else
			return TRAP;
	}
	return OK;
}

Status PMTable::remove(VirtualAddress address)
{
	if (pmt1[PMT1ENTRY(address)]->pmt2)
	{
		pmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].physicalAddr = nullptr;
		pmt1[PMT1ENTRY(address)]->pmt2[PMT2ENTRY(address)].free = true;

		if (!checkPMT2HasContents(address))
		{
			PMTSlabAlloc::free((PhysicalAddress)(pmt1[PMT1ENTRY(address)]));
			pmt1[PMT1ENTRY(address)] = nullptr;
		}
		return OK;
	}
	else return TRAP;
}

PMTable::~PMTable()
{
	for (int i = 0; i < 256; i++)
		if (pmt1[i]) {
			PMTSlabAlloc::free(pmt1[i]);
			pmt1[i] = nullptr;
		}
	PMTSlabAlloc::free(pmt1);
}

PMTable::PMTable()
{
	pmt1 = (PMT2**)(PMTSlabAlloc::allocate());
}
