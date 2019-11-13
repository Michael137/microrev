#include <sysexits.h>
#include <string>
#include <iostream>
#include <cassert>

#include "pmc_utils.h"

namespace pmc_utils {

// PMC_MODE_TC: Per-processor counter
Counter::Counter() : cset(), pmc_values(), pmcid(), mode(PMC_MODE_TC) {}

void pmc_begin(Counter& counter)
{
	counter.start();
}

void Counter::start()
{
	if(pmc_init() < 0)
		err(EX_OSERR, "Cannot initialize pmc(3)");

	for(auto& ctr : this->cset)
	{
		if(pmc_allocate(ctr.c_str(), mode, 0, PMC_CPU_ANY, &(this->pmcid), 0) < 0)
		{
			err(EX_OSERR, "Cannot allocate counter \"%s\"", ctr.c_str());
		}
	}

	if (pmc_start(this->pmcid) < 0)
		err(EX_OSERR, "Cannot start pmc");
}

void pmc_end(Counter& counter)
{
	counter.read();
	counter.stats();
}

void Counter::stats()
{
	assert(this->cset.size() == this->pmc_values.size());
	for(int i = 0; i < this->cset.size(); ++i)
	{
		std::cout << this->cset[i] << ": " << this->pmc_values[i] << '\n';
	}
	std::cout << std::endl;
}

void Counter::add(std::string counter_name)
{
	this->cset.push_back(counter_name);
}

void Counter::read()
{
	uint64_t v;
	for(auto& e : this->cset)
	{
		if(pmc_read(this->pmcid, &v) < 0)
			err(EX_OSERR, "Cannot read pmc");
		this->pmc_values.push_back(v);
	}
}

} // namespace pmc_utils
