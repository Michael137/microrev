#include <chrono>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

#include "constants.h"
#include "counter.h"
#include "thread_utils.h"

using namespace pcnt;

#define FIVE( a ) a a a a a
#define TEN( a ) FIVE( a ) FIVE( a )
#define FIFTY( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a )
#define HUNDRED( a ) FIFTY( a ) FIFTY( a )

/*
 * With predicatble stride i.e. pre-fetcher
 * will trigger
 *
 * From:
 * https://github.com/foss-for-synopsys-dwc-arc-processors/lmbench/blob/master/src/lib_mem.c#L177
 */
char** __attribute__( ( optimize( "0" ) ) )
init_stride( uint64_t size, uint64_t stride )
{
	char* arr = (char*)malloc( size * sizeof( char ) );

	char** head = (char**)arr;
	char** iter = head;

	for( uint64_t i = 0; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		*iter = &arr[i + stride];

		iter += ( stride / sizeof( iter ) );
	}

	// Loop back end of linked list to the head
	*iter = (char*)head;

	return iter;
}

void __attribute__( ( optimize( "0" ) ) )
time_rd_latency( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	const int accesses = 10000000;

	char** iter = init_stride( size, stride );

	pc.start();
	uint64_t start = rdtsc();
	// Pointer-chase through linked list
	for( int i = 0; i < accesses; ++i )
	{
		// Unroll loop partially to reduce loop overhead
		HUNDRED( iter = ( (char**)*iter ); )
	}

	uint64_t end = rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back( end - start );
}

// Vary strides
void test8( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _8B ); }
void test16( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _16B ); }
void test32( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _32B ); }
void test64( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _64B ); }
void test128( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _128B ); }
void test256( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _256B ); }
void test512( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _512B ); }
void test1024( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, _1KB ); }

int main( int argc, char** argv )
{
	/* Should allow us to see cache sizes */

	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

	std::vector<Sched> vec{
	    { 1, std::function<decltype( test8 )>{ test8 }, {}, "8B" },
	    { 1, std::function<decltype( test8 )>{ test16 }, {}, "16B" },
	    { 1, std::function<decltype( test8 )>{ test32 }, {}, "32B" },
	    { 1, std::function<decltype( test8 )>{ test64 }, {}, "64B" },
	    { 1, std::function<decltype( test8 )>{ test128 }, {}, "128B" },
	    { 1, std::function<decltype( test8 )>{ test256 }, {}, "256B" },
	    { 1, std::function<decltype( test8 )>{ test512 }, {}, "512B" },
	    { 1, std::function<decltype( test8 )>{ test1024 }, {}, "1KB" },
	};
	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec, 3 /* warmup */ );

	for( auto& c: counters )
		c.print_stats();

	return 0;
}
