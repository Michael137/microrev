#include <err.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#include <pthread.h>
#include <stdlib.h>
#include <functional>
#include <vector>

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

void do_flops()
{
	float x = 0.0;
	for( int i = 0; i < 100000000; ++i )
		x *= 0.2;
}
int main( int argc, char* argv[] )
{
	std::cout
	    << ">>>> TEST: reading counters on multiple threads using libpmc/PAPI"
	    << std::endl;
#ifdef WITH_PMC

	auto FreeBSDCounters = pcnt::CounterMap["FreeBSD"];

	pcnt::CounterBenchmark<pcnt::PMCCounter> cbench{
	    std::function<void( void )>{ do_flops } };

	cbench.counters_on_cores<std::vector<std::string>>(
	    FreeBSDCounters["dcache"] );

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

	std::vector<int> events{ PAPI_TOT_INS };
	pcnt::PAPIHLCounter pcounter( events );

	/* Start counting events */
	pcounter.start();
	do_flops();
	pcounter.read();
	pcounter.stats();
	return 0;

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	std::vector<int> events{
	    PAPI_TOT_INS,
	    PAPI_FP_INS,
	    PAPI_TOT_CYC,
	};

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	cbench.counters_on_cores<std::vector<int>&>(
	    events, std::function<void( void )>{ do_flops } );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
