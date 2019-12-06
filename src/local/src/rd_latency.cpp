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

// See test/rd_latency.cpp for explanation
char* init_stride( uint64_t size, uint64_t stride = 64 )
{
	char* arr = (char*)malloc( size * sizeof( char ) );

	int i;
	for( i = stride; i < size; i += stride )
	{
		*(char**)&arr[i - stride] = (char*)&arr[i];
	}
	*(char**)&arr[i - stride] = (char*)&arr[0];

	return arr;
}

void OPT0 time_rd_latency( PAPILLCounter& pc, uint64_t size,
                           uint64_t stride = 64 )
{
	const int accesses = 1000000;

	char* arr   = init_stride( size, stride );
	char** iter = (char**)arr;

	pc.start();
	uint64_t start = rdtsc();
	for( int i = 0; i < accesses; ++i )
		HUNDRED( iter = ( (char**)*iter ); )

	uint64_t end = rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back( end - start );

	free(arr);
}

// Stride: 64 bytes
void test4_64( PAPILLCounter& pc ) { time_rd_latency( pc, _4KB, 64 ); }
void test8_64( PAPILLCounter& pc ) { time_rd_latency( pc, _8KB, 64 ); }
void test16_64( PAPILLCounter& pc ) { time_rd_latency( pc, _16KB, 64 ); }
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
	/* Should allow us to see cache sizes */

	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

	std::vector<Sched> vec{
	    { 1, std::function<decltype( test32_64 )>{ test4_64 }, {}, "4KB" },
	    { 1, std::function<decltype( test32_64 )>{ test8_64 }, {}, "8KB" },
	    { 1, std::function<decltype( test32_64 )>{ test16_64 }, {}, "16KB" },
	    { 1, std::function<decltype( test32_64 )>{ test32_64 }, {}, "32KB" },
	    { 1, std::function<decltype( test32_64 )>{ test64_64 }, {}, "64KB" },
	    { 1, std::function<decltype( test32_64 )>{ test128_64 }, {}, "128KB" },
	    { 1, std::function<decltype( test32_64 )>{ test256_64 }, {}, "256KB" },
	    { 1, std::function<decltype( test32_64 )>{ test512_64 }, {}, "512KB" },
	    { 1, std::function<decltype( test32_64 )>{ test1024_64 }, {}, "1MB" },
	    { 1, std::function<decltype( test32_64 )>{ test1024_64 }, {}, "2MB" },
	    { 1, std::function<decltype( test32_64 )>{ test2048_64 }, {}, "4MB" } };
	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );

	for( auto& c: counters )
		c.print_stats();

	return 0;
}
