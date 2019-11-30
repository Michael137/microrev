#include <chrono>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

#include "constants.h"
#include "thread_utils.h"

using namespace pcnt;

#define FIVE( a ) a a a a a
#define TEN( a ) FIVE( a ) FIVE( a )
#define FIFTY( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a )
#define HUNDRED( a ) FIFTY( a ) FIFTY( a )

#define B2MB( b ) ( (double)b ) / 1024 / 1024

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
char** init_stride( uint64_t size )
{
	char* arr = (char*)malloc( size * sizeof( char ) );

	uint64_t stride = 64;

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

uint64_t time_rd_latency( uint64_t size )
{
	const int accesses = 1000000;

	char** iter = init_stride( size );

//	auto start = std::chrono::high_resolution_clock::now();

	uint64_t start = rdtsc();

	// Pointer-chase through linked list
	for( int i = 0; i < accesses; ++i )
	{
		// Unroll loop partially to reduce loop overhead
		HUNDRED( iter = ( (char**)*iter ); )
	}

	uint64_t end = rdtsc();
	//auto end = std::chrono::high_resolution_clock::now();

//	return std::chrono::duration_cast<std::chrono::nanoseconds>( end
//	                                                             - start )
//	    .count();

	return end - start;
}

int main( int argc, char** argv )
{
	printf( "%f: %lu\n", B2MB( _4KB ), time_rd_latency( _4KB ) );
	printf( "%f: %lu\n", B2MB( _8KB ), time_rd_latency( _8KB ) );
	printf( "%f: %lu\n", B2MB( _16KB ), time_rd_latency( _16KB ) );
	printf( "%f: %lu\n", B2MB( _32KB ), time_rd_latency( _32KB ) );
	printf( "%f: %lu\n", B2MB( _64KB ), time_rd_latency( _64KB ) );
	printf( "%f: %lu\n", B2MB( _128KB ), time_rd_latency( _128KB ) );
	printf( "%f: %lu\n", B2MB( _256KB ), time_rd_latency( _256KB ) );
	printf( "%f: %lu\n", B2MB( _512KB ), time_rd_latency( _512KB ) );
	printf( "%f: %lu\n", B2MB( _1MB ), time_rd_latency( _1MB ) );
	printf( "%f: %lu\n", B2MB( _2MB ), time_rd_latency( _2MB ) );
	printf( "%f: %lu\n", B2MB( _2MB ), time_rd_latency( _2MB ) );
	printf( "%f: %lu\n", B2MB( _4MB ), time_rd_latency( _4MB ) );
	return 0;
}
