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
	//         S0                   S1
	// |-------|-------|####|-------|-------|
	// |-- 0 --|-- 2 --|####|-- 1 --|-- 3 --|
	// |-------|-------|    |-------|-------|
	// |-- 4 --|-- 6 --|    |-- 5 --|-- 7 --|
	// |-------|-------|    |-------|-------|
	// |-- 8 --|--10 --|    |-- 9 --|--11 --|
	// |-------|-------|    |-------|-------|
	// |--12 --|--14 --|    |--13 --|--15 --|
	// |-------|-------|    |-------|-------|
	//
	// Write on core 0 (setup)
	// Read core 1 (core_src)
	// Read on core 8 (global0)
	// Flush on core 1
	// Read on core 10 (global1) <--- Measure read latency
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
	vec.push_back( Sched{ core_c, std::function<decltype( reader )>{ reader },
	                      counters } );

	auto measured
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );

	for( auto& m: measured )
		if( m.cset.size() != 0 )
			results.push_back( m.vec_cycles_measured[0] );
}

void read_from_invalid_test( std::vector<std::string> counters,
                   std::vector<uint64_t>& results )
{
	CounterBenchmark<PAPILLCounter> cbench;
	std::vector<Sched> vec;

	// Core a and c are on different sockets
	int core_a = core_src, core_b, core_c;

	// Write on core 0 <--- setup
	// Flush on core 1 (core_src)
	// Flush on core 10 (global1)
	// Read on core 10 (global1) <--- Measure read latency
	core_b = core_global1;

	// Flush cores on socket 0 and 1
	vec.push_back(
	    Sched{ core_a, std::function<decltype( flusher )>{ flusher }, {} } );
	vec.push_back(
	    Sched{ core_b, std::function<decltype( flusher )>{ flusher }, {} } );

	// Finally, read on socket 1
	vec.push_back(
	    Sched{ core_b, std::function<decltype( reader )>{ reader }, counters } );

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

	/* Hypothesis:
	 *     Flushing a cache line on one socket will flush it on all other sockets
	 * Test:
         *     If the above is true then a flush on socket 0 followed by a read
	 *     on socket 1 should effectively have the same cost as a read from
	 *     an invalid cache line (i.e., read from main memory)
	 *
	 * Sweep:
	 *     We perform the test for increasing working set (i.e., shared data) size
         */
	INIT_ARCH_CFG( 1, 3, 5, 8, 10, _32KB, _64B, _256KB )

	std::vector<std::string> counters
	    = { "perf::L1-DCACHE-LOAD-MISSES",
	        "perf::L1-DCACHE-LOADS" };

	std::vector<uint64_t> flush_cycles;
	std::vector<uint64_t> invalid_cycles;

	setup( shared_data_size );

	for( int i = 0; i < 1000; ++i )
		flusher_test( counters, flush_cycles );

	for( int i = 0; i < 1000; ++i )
		read_from_invalid_test( counters, invalid_cycles );

	BENCHMARK_END();

	std::cout << "Flushing on one socket: "
	          << std::accumulate( flush_cycles.begin(), flush_cycles.end(), 0 ) / flush_cycles.size()
	          << std::endl;

	std::cout << "Flushing on both sockets: "
	          << std::accumulate( invalid_cycles.begin(), invalid_cycles.end(), 0 ) / invalid_cycles.size()
	          << std::endl;

#endif // !WITH_PAPI_LL

	return 0;
}
