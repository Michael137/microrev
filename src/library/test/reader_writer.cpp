#include <inttypes.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

#include "constants.h"
#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

volatile int* shared = NULL;
volatile int* flags = NULL;
static const int shared_size = _32KB;
static const int cache_line_size = 64;
static const int accesses = shared_size / cache_line_size;

void OPT0 reader( pcnt::PAPILLCounter& pc )
{
	uint64_t start, end;
	pc.start();
	start   = pcnt::rdtsc();	
	for(int i = 0; i < shared_size; i += cache_line_size)
	{
		while(__sync_lock_test_and_set(&flags[i / cache_line_size], 0));
		int a = shared[i];
	}
	end = pcnt::rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

void OPT0 writer( pcnt::PAPILLCounter& pc )
{
	uint64_t start, end;
	pc.start();
	start = pcnt::rdtsc();
	for(int i = 0; i < shared_size; i += cache_line_size)
	{
		shared[i] = 42;
		__sync_fetch_and_add( &flags[i / cache_line_size], -1 );
	}
	end = pcnt::rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

int main( int argc, char* argv[] )
{
	std::cout << ">>>> TEST: reader writer scenario"
	          << std::endl;
#if defined( WITH_PAPI_LL )

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

	shared = (int*) malloc(sizeof(int) * shared_size);
	flags = (int*) malloc(sizeof(int) * accesses);

	for( int i = 0; i < shared_size; ++i)
		shared[i] = 1;

	for( int i = 0; i < accesses; ++i)
		flags[i] = 0;

	Sched core_1 = Sched{
	    1,
	    std::function<decltype( reader )>{ reader },
	    { "PAPI_FP_INS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};
	Sched core_2 = Sched{
	    2,
	    std::function<decltype( writer )>{ writer },
	    { "PAPI_FP_INS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};

	std::vector<Sched> vec{ core_1, core_2};

	auto counters = cbench.counters_with_schedule<std::vector<std::string>>( vec );

	for(auto& c : counters)
		c.print_stats();

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
