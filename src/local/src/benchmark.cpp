#include <stdlib.h>

#include "counter.h"
#include "counters.h"

void contig_access_test()
{
	int arr[10000] = { 0 };
	for( int i = 1; i < 10000; ++i )
		arr[i] = arr[i] - 1;
}

int main( int argc, char* argv[] )
{
	auto FreeBSDCounters = pcnt::CounterMap["FreeBSD"];
	//pcnt::PFMCounter counter( FreeBSDCounters["dcache"] );
	//	counter.start( counter );
	//
	//	contig_access_test();
	//
	//	counter.read();
	//	counter.stats();

	return 0;
}
