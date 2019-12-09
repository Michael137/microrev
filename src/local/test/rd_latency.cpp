#include <chrono>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

#include "constants.h"
#include "shuffle.h"
#include "thread_utils.h"

using namespace pcnt;

#define FIVE( a ) a a a a a
#define TEN( a ) FIVE( a ) FIVE( a )
#define FIFTY( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a )
#define HUNDRED( a ) FIFTY( a ) FIFTY( a )

#define B2MB( b ) ( (double)b ) / 1024 / 1024

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
	for( int i = 0; i < size; ++i )
		arr[i] = 0;

	uint64_t stride = 64;

	std::vector<char*> rndarray;
	for( uint64_t i = 0; i < size; i += stride )
	{
		rndarray.push_back( (char*)&arr[i] );
	}

	char** head        = (char**)arr;
	char** shared_iter = head;
	shuffle_array<char*>( rndarray );

	for( uint64_t i = 0; i < size; i += stride )
	{
		*shared_iter = *(char**)&rndarray[i / stride];

		shared_iter += ( stride / sizeof( shared_iter ) );
	}
	*shared_iter = (char*)head;

	//	int i;
	//	for( i = stride; i < size; i += stride )
	//	{
	//		// Arrange linked list such that:
	//		// Pointer at arr[i] == Pointer at arr[i + stride]
	//		*(char**)&arr[i - stride] = (char*)&arr[i];
	//	}
	//	// Loop back end of linked list to the head
	//	*(char**)&arr[i - stride] = (char*)&arr[0];

	return shared_iter;
}

uint64_t time_rd_latency( uint64_t size )
{
	const int accesses = 1000000;

	// char* arr   = init_stride( size );
	// char** iter = (char**)arr;
	char** iter = init_stride( size );

	uint64_t start = rdtsc();

	// Pointer-chase through linked list
	for( int i = 0; i < accesses; ++i )
	{
		// Unroll loop partially to reduce loop overhead
		HUNDRED( iter = ( (char**)*iter ); )
	}

	uint64_t end = rdtsc();

	// free( *iter );

	return end - start;
}

int main( int argc, char** argv )
{
	std::cout << ">>>> TEST: read latency while pointer chasing linked list "
	             "with a predictable stride"
	          << std::endl;

	time_rd_latency( _64B );
	time_rd_latency( _64B );
	uint64_t start = rdtsc();
	uint64_t end   = rdtsc();
	printf( "Noise: %lu\n", end - start );

	printf( "%f MB: %lu\n", B2MB( _64B ), time_rd_latency( _64B ) );
	printf( "%f MB: %lu\n", B2MB( _128B ), time_rd_latency( _128B ) );
	printf( "%f MB: %lu\n", B2MB( _256B ), time_rd_latency( _256B ) );
	printf( "%f MB: %lu\n", B2MB( _512B ), time_rd_latency( _512B ) );
	printf( "%f MB: %lu\n", B2MB( _1KB ), time_rd_latency( _1KB ) );
	printf( "%f MB: %lu\n", B2MB( _2KB ), time_rd_latency( _2KB ) );
	printf( "%f MB: %lu\n", B2MB( _4KB ), time_rd_latency( _4KB ) );
	printf( "%f MB: %lu\n", B2MB( _8KB ), time_rd_latency( _8KB ) );
	printf( "%f MB: %lu\n", B2MB( _16KB ), time_rd_latency( _16KB ) );
	printf( "%f MB: %lu\n", B2MB( _32KB ), time_rd_latency( _32KB ) );
	printf( "%f MB: %lu\n", B2MB( _64KB ), time_rd_latency( _64KB ) );
	printf( "%f MB: %lu\n", B2MB( _128KB ), time_rd_latency( _128KB ) );
	printf( "%f MB: %lu\n", B2MB( _256KB ), time_rd_latency( _256KB ) );
	printf( "%f MB: %lu\n", B2MB( _512KB ), time_rd_latency( _512KB ) );
	printf( "%f MB: %lu\n", B2MB( _1MB ), time_rd_latency( _1MB ) );
	printf( "%f MB: %lu\n", B2MB( _2MB ), time_rd_latency( _2MB ) );
	printf( "%f MB: %lu\n", B2MB( _4MB ), time_rd_latency( _4MB ) );
	printf( "%f MB: %lu\n", B2MB( _8MB ), time_rd_latency( _8MB ) );
	printf( "%f MB: %lu\n", B2MB( _16MB ), time_rd_latency( _16MB ) );

	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
