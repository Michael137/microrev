#include <sysexits.h>
#include <string>
#include <iostream>

#include "pmc_utils.h"

static pmc_id_t pmcid;

void pmc_begin()
{
	std::string counter; // TODO: set of counters should be argument
	enum pmc_mode mode;

	if(pmc_init() < 0)
		err(EX_OSERR, "Cannot initialize pmc(3)");

	// Per-processor counter
	// List of counters: `pmccontrol -L`
	mode = PMC_MODE_TC;
	counter = "icache.hit";
	if(pmc_allocate(counter.c_str(), mode, 0, PMC_CPU_ANY, &pmcid, 0) < 0)
	{
		err(EX_OSERR, "Cannot allocate counter \"%s\"", counter.c_str());
	}
}

void pmc_setup()
{
	if (pmc_start(pmcid) < 0)
		err(EX_OSERR, "Cannot start pmc");
}

void pmc_end()
{
	pmc_value_t v;
	if(pmc_read(pmcid, &v) < 0)
		err(EX_OSERR, "Cannot read pmc");
	std::cout << v << std::endl;
}
