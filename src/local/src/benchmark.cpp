#include <stdlib.h>

#include "pmc_counters.h"
#include "pmc_utils.h"

void contig_access_test()
{
	int arr[10000] = {0};
	for( int i = 1; i < 10000; ++i )
		arr[i] = arr[i] - 1;
}

int main( int argc, char* argv[] )
{
	auto FreeBSDCounters = pmc_utils::CounterMap["FreeBSD"];
	pmc_utils::Counter counter( FreeBSDCounters["dcache"] );
	pmc_utils::pmc_begin( counter );

	contig_access_test();

	pmc_utils::pmc_end( counter );

	return 0;
}
