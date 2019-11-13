#include <stdlib.h>

#include "pmc_utils.h"

int main(int argc, char *argv[])
{
	pmc_id_t pmcid;
	pmc_utils::Counter counter;
	counter.add("icache.hit");
	counter.add("icache.misses");
	counter.add("icache.ifetch_stall");
	pmc_utils::pmc_begin(counter);
	pmc_utils::pmc_end(counter);

	return 0;
}
