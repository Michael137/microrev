#include <inttypes.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark_helpers.h"
#include "counter.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

BENCHMARK_INIT();

void flusher_test( std::vector<std::string> counters,
                   std::vector<uint64_t>& results )
{
	CounterBenchmark<PAPILLCounter> cbench;
	std::vector<Sched> vec;

	// Core a and c are on different sockets
	int core_a = core_src, core_b, core_c;

	// Read across sockets
	core_b = core_global0;
	core_c = core_global1;

	// Flush all cache lines on all cores
	vec.push_back(
	    Sched{ core_a, std::function<decltype( flusher )>{ flusher }, {} } );
	vec.push_back(
	    Sched{ core_b, std::function<decltype( flusher )>{ flusher }, {} } );
	vec.push_back(
	    Sched{ core_c, std::function<decltype( flusher )>{ flusher }, {} } );

	// Socket 1 now in S-state
	init_state( vec, S_STATE, core_a, core_b );

	// Now flush socket 1 and read from socket 2
	vec.push_back(
	    Sched{ core_a, std::function<decltype( flusher )>{ flusher }, {} } );
	vec.push_back( Sched{ core_c, std::function<decltype( flusher )>{ reader },
	                      counters } );

	auto measured
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );

	for( auto& m: measured )
		if( m.cset.size() != 0 )
			results.push_back( m.vec_cycles_measured[0] );
}

int main( int argc, char* argv[] )
{
#ifdef WITH_PMC

#	error "TODO: implement this test using PMC"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC

	INIT_ARCH_CFG( 1, 2, 3, 1, 2, _32KB, _64B, _256KB )

	std::vector<std::string> counters
	    = { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
	        "perf::L1-DCACHE-LOADS" };

	std::vector<uint64_t> cycles;

	setup( shared_data_size );
	for( int i = 0; i < 1000; ++i )
		flusher_test( counters, cycles );
	free( (void*)shared_data );

	std::cout << "Average cycles: "
	          << std::accumulate( cycles.begin(), cycles.end(), 0 ) / cycles.size()
	          << std::endl;

#endif // !WITH_PAPI_LL

	return 0;
}
