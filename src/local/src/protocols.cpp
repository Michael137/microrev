#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <vector>

#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

#define FIVE( a ) a a a a a
#define TEN( a ) FIVE( a ) FIVE( a )
#define FIFTY( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a )
#define HUNDRED( a ) FIFTY( a ) FIFTY( a )

using namespace pcnt;

volatile char* shared_data         = nullptr;
volatile char** shared_iter        = nullptr;
volatile uint64_t shared_data_size = _16KB;

// Create linked list
void __attribute__( ( optimize( "0" ) ) )
writer_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	shared_data = (char*)malloc( size * sizeof( char ) );

	volatile char** head = (volatile char**)shared_data;
	shared_iter          = head;

	for( uint64_t i = 0; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		*shared_iter = &shared_data[i + stride];

		shared_iter += ( stride / sizeof( shared_iter ) );
	}

	// Loop back end of linked list to the head
	*shared_iter = (char*)head;

	// TODO: random shuffle the entries
}

void __attribute__( ( optimize( "0" ) ) )
reader_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	const int accesses = 1000000;

	pc.start();
	uint64_t start = rdtsc();
	// Pointer-chase through linked list
	for( int i = 0; i < accesses; ++i )
	{
		// Unroll loop partially to reduce loop overhead
		HUNDRED( shared_iter = ( (volatile char**)*shared_iter ); )
	}
	uint64_t end = rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

void reader( PAPILLCounter& pc ) { reader_( pc, _32KB, 64 ); }

void writer( PAPILLCounter& pc ) { writer_( pc, _32KB, 64 ); }

int main( int argc, char* argv[] )
{
	std::cout
	    << ">>>> TEST: force caches into M/E/S/I/F and measure performance"
	    << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC

	/* thread 1: write to shared_data
	 * thread 2: read from shared_data (now cache in core 1 and 2 should be in S
state)
	 * thread 3 to N: read from shared_data (in MESIF protocol the traffic
produced should be the same as if only a single core is asking for the data) */

	shared_data = (char*)malloc( sizeof( char ) * shared_data_size );

	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

	Sched core_1 = Sched{ 1 /* core id */,
	                      std::function<decltype( writer )>{ writer },
	                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } };
	Sched core_2 = Sched{ 2 /* core id */,
	                      std::function<decltype( reader )>{ reader },
	                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } };

	std::vector<Sched> vec{ core_1, core_2 };

	cbench.counters_with_priority_schedule<std::vector<std::string>>( vec );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
