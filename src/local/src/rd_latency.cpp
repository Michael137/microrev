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

static std::random_device rd;
static std::mt19937 gen( rd() );
static std::uniform_int_distribution<> dist( 1,
                                             std::numeric_limits<int>::max() );

int gen_num() { return dist( gen ); }

/*
 * With predicatble stride i.e. pre-fetcher
 * will trigger
 *
 * From:
 * https://github.com/foss-for-synopsys-dwc-arc-processors/lmbench/blob/master/src/lib_mem.c#L177
 */
char** init_stride( uint64_t size, uint64_t stride )
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

void time_rd_latency( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	const int accesses = 1000000;

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
	pc.cycles_measured = end - start;
}

// Stride: 64 bytes
void test32_64( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, 64 ); }
void test64_64( PAPILLCounter& pc ) { time_rd_latency( pc, _64KB, 64 ); }
void test128_64( PAPILLCounter& pc ) { time_rd_latency( pc, _128KB, 64 ); }
void test256_64( PAPILLCounter& pc ) { time_rd_latency( pc, _256KB, 64 ); }
void test512_64( PAPILLCounter& pc ) { time_rd_latency( pc, _512KB, 64 ); }
void test1024_64( PAPILLCounter& pc ) { time_rd_latency( pc, _1MB, 64 ); }
void test2048_64( PAPILLCounter& pc ) { time_rd_latency( pc, _2MB, 64 ); }

// Stride: 128 bytes
void test32_128( PAPILLCounter& pc ) { time_rd_latency( pc, _32KB, 128 ); }
void test64_128( PAPILLCounter& pc ) { time_rd_latency( pc, _64KB, 128 ); }
void test128_128( PAPILLCounter& pc ) { time_rd_latency( pc, _128KB, 128 ); }
void test256_128( PAPILLCounter& pc ) { time_rd_latency( pc, _256KB, 128 ); }
void test512_128( PAPILLCounter& pc ) { time_rd_latency( pc, _512KB, 128 ); }
void test1024_128( PAPILLCounter& pc ) { time_rd_latency( pc, _1MB, 128 ); }
void test2048_128( PAPILLCounter& pc ) { time_rd_latency( pc, _2MB, 128 ); }

int main( int argc, char** argv )
{
	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

	Sched core_1
	    = Sched{ 1,
	             std::function<decltype( test32_64 )>{ test32_64 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	Sched core_2
	    = Sched{ 2,
	             std::function<decltype( test32_64 )>{ test64_64 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	Sched core_3
	    = Sched{ 3,
	             std::function<decltype( test32_64 )>{ test2048_64 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };

	std::vector<Sched> vec{ core_1, core_2, core_3 };
	cbench.counters_with_schedule<std::vector<std::string>>( vec );

	return 0;
}
