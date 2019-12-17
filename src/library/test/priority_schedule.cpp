#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "counter.h"
#include "thread_utils.h"
#include "time_utils.h"

#if defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

void do_flops( pcnt::PAPILLCounter& pc )
{
	uint64_t start, end;
	pc.start();
	start   = pcnt::rdtsc();
	float x = 0.0;
	for( int i = 0; i < 10000; ++i )
		x *= 0.2;
	end = pcnt::rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

int main( int argc, char* argv[] )
{
	std::cout << ">>>> TEST: schdule counter benchmarks in series" << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PMC"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

	Sched core_1 = Sched{
	    1,
	    std::function<decltype( do_flops )>{ do_flops },
	    { "PAPI_FP_INS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};
	Sched core_2 = Sched{
	    2,
	    std::function<decltype( do_flops )>{ do_flops },
	    { "PAPI_L2_DCM", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};
	Sched core_1a = Sched{
	    1,
	    std::function<decltype( do_flops )>{ do_flops },
	    { "PAPI_TOT_INS", "PAPI_L2_DCH", "PAPI_TOT_CYC" },
	};

	std::vector<Sched> vec{ core_1, core_2, core_1a };

	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );

	for( auto& c: counters )
		c.print_stats();

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
