#include <err.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

//#include <perfmon/pfmlib.h>
//#include <perfmon/pfmlib_perf_event.h>
#include "counter.h"
#include "counters.h"

#include <papi.h>

#define NUM_FLOPS 10000
#define NUM_EVENTS 1

void do_flops( int flops )
{
	float x = 0.0;
	for( int i = 0; i < flops * flops; ++i )
		x *= 0.2;
}

int main( int argc, char* argv[] )
{
#ifdef __FreeBSD__
	auto FreeBSDCounters = pcnt::CounterMap["FreeBSD"];
	pcnt::PFMCounter counter( FreeBSDCounters["dcache"] );
	counter.start( counter );
	do_flops();
	counter.read();
	counter.stats();
#else
	std::vector<int> events{ PAPI_TOT_INS };
	pcnt::PAPIHLCounter pcounter( events );

	/* Start counting events */
	pcounter.start();

	do_flops( NUM_FLOPS );

	pcounter.read();
	pcounter.stats();
#endif

	return 0;
}
