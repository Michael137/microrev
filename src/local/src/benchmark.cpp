#include <stdlib.h>

#include "pmc_counters.h"
#include "pmc_utils.h"

int main( int argc, char* argv[] )
{
	pmc_id_t pmcid;
	auto FreeBSDCounters = pmc_utils::CounterMap["FreeBSD"];
	pmc_utils::Counter counter( FreeBSDCounters["icache"] );
	pmc_utils::pmc_begin( counter );
	pmc_utils::pmc_end( counter );

	return 0;
}
