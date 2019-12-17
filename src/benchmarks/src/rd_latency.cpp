#include <chrono>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

#include "constants.h"
#include "counter.h"
#include "shuffle.h"
#include "thread_utils.h"
#include "time_utils.h"

using namespace pcnt;

#define FIVE( a ) a a a a a
#define TEN( a ) FIVE( a ) FIVE( a )
#define FIFTY( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a )
#define HUNDRED( a ) FIFTY( a ) FIFTY( a )

#define WRITE_BLOCK( li, ad, tmp )                                             \
	do                                                                         \
	{                                                                          \
		*( li + ad )     = tmp + ad;                                           \
		*( li + ad + 1 ) = tmp + ad + 1;                                       \
		*( li + ad + 2 ) = tmp + ad + 2;                                       \
		*( li + ad + 3 ) = tmp + ad + 3;                                       \
		*( li + ad + 4 ) = tmp + ad + 4;                                       \
		*( li + ad + 5 ) = tmp + ad + 5;                                       \
		*( li + ad + 6 ) = tmp + ad + 6;                                       \
		*( li + ad + 7 ) = tmp + ad + 7;                                       \
	} while( 0 );

char* arr = NULL;

// See test/rd_latency.cpp for explanation
char* init_stride( uint64_t size, uint64_t stride = 64 )
{
	int i;
	for( i = stride; i < size; i += stride )
	{
		*(char**)&arr[i - stride] = (char*)&arr[i];
	}
	*(char**)&arr[i - stride] = (char*)&arr[0];

	char** iter       = (char**)arr;
	char* local_arr   = (char*)malloc( size * sizeof( char ) );
	char** local_iter = (char**)local_arr;
	char* tmp;
	uint64_t start, end;
	uint64_t blksize        = _128KB;
	uint64_t line_cnt       = blksize / stride;
	uint64_t outer_line_cnt = size / blksize;
	int64_t addr_diff       = ( iter - local_iter );
	for( uint64_t i = 0; i < size; ++i )
		local_arr[i] = 0;
	set_shuffled_linked_list( local_arr, size, stride );
	for( uint64_t i = 0; i < outer_line_cnt; i++ )
	{
		std::cout << "producing!" << std::endl;
		for( uint64_t j = 0; j < line_cnt; j++ )
		{
			tmp = *( local_iter );
			//WRITE_BLOCK( local_iter, addr_diff, tmp );
			local_iter = (char**)tmp;
		}
		std::cout << "produced!" << std::endl;
	}

	//char** iter1             = (char**)arr;
	//char* local_arr1         = (char*)malloc( size * sizeof( char ) );
	//char** local_iter1       = (char**)local_arr;
	//uint64_t blksize1        = _128KB;
	//uint64_t line_cnt1       = blksize / stride;
	//uint64_t outer_line_cnt1 = size / blksize;
	//addr_diff                = ( local_iter1 - iter1 );
	//start                    = rdtsc();
	//for( uint64_t i = 0; i < outer_line_cnt1; i++ )
	//{
	//	std::cout << "consuming!" << std::endl;
	//	for( uint64_t j = 0; j < line_cnt; j++ )
	//	{
	//		tmp                    = *( iter1 );
	//		*( iter1 + addr_diff ) = tmp + addr_diff;
	//		iter                   = (char**)tmp;
	//	}
	//}

	return arr;
}

void OPT0 time_rd_latency( PAPILLCounter& pc, uint64_t size,
                           uint64_t stride = 64 )
{
	const int accesses = 1000000;

	char* arr = init_stride( size, stride );
	//	char** iter = (char**)arr;

	//	pc.start();
	//	uint64_t start = rdtsc();
	//	for( int i = 0; i < accesses; ++i )
	//		HUNDRED( iter = ( (char**)*iter ); )
	//
	//	uint64_t end = rdtsc();
	//	pc.read();
	//	pc.vec_cycles_measured.push_back( end - start );
	//
	//	free( arr );
}

// Stride: 64 bytes
void test64B_64( PAPILLCounter& pc ) { time_rd_latency( pc, _64B, 64 ); }
void test128B_64( PAPILLCounter& pc ) { time_rd_latency( pc, _128B, 64 ); }
void test256B_64( PAPILLCounter& pc ) { time_rd_latency( pc, _256KB, 64 ); }
void test512B_64( PAPILLCounter& pc ) { time_rd_latency( pc, _512B, 64 ); }
void test1_64( PAPILLCounter& pc ) { time_rd_latency( pc, _1KB, 64 ); }
void test2_64( PAPILLCounter& pc ) { time_rd_latency( pc, _2KB, 64 ); }
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

int main( int argc, char** argv )
{
	/* Should allow us to see cache sizes */
	arr = (char*)malloc( _128KB * sizeof( char ) );

	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

	std::vector<Sched> vec{
	    { 1,
	      std::function<decltype( test32_64 )>{ test128_64 },
	      {},
	      "128KB" } };
	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>( vec,
	                                                                        0 );

	for( auto& c: counters )
		c.stats();

	return 0;
}
