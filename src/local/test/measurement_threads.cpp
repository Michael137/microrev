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
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

void doflops( pcnt::PAPILLCounter& pc )
{
	uint64_t start, end;
	pc.start();
	start   = pcnt::rdtsc();
	float x = 0.0;
	for( int i = 0; i < 100000; ++i )
		x *= 0.2;
	end = pcnt::rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

void no_multi_measurement()
{
	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

	Sched core_1 = Sched{
	    1,
	    std::function<decltype( doflops )>{ doflops },
	    { "PAPI_FP_INS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};

	std::vector<Sched> vec = { core_1 };
	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );

	for( auto& cnt: counters )
		cnt.stats();
}

void multi_measurement()
{
	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

	Sched core_1 = Sched{
	    1,
	    std::function<decltype( doflops )>{ doflops },
	    { "perf::LLC-LOADS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};

	Sched measure_1 = Sched{
	    2,
	    std::function<decltype( doflops )>{ doflops },
	    { "perf::LLC-LOADS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};
	Sched measure_2 = Sched{
	    2,
	    std::function<decltype( doflops )>{ doflops },
	    { "perf::LLC-LOADS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};

	core_1.measurement_scheds = { measure_1, measure_2 };

	std::vector<Sched> vec = { core_1 };
	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );

	for( auto& cnt: counters )
		cnt.stats();
}

int main( int argc, char* argv[] )
{
	std::cout
	    << ">>>> TEST: perform benchmarks on one core and measure on others"
	    << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PMC"

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	multi_measurement();
	no_multi_measurement();

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
