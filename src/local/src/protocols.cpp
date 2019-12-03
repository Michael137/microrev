#include <err.h>
#include <immintrin.h>
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
#include "shuffle.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

#define M_STATE 0
#define E_STATE 1
#define S_STATE 2
#define I_STATE 3
#define O_STATE 4
#define F_STATE 5

using namespace pcnt;
using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

uint64_t CACHELINE_SIZE = 64;

typedef enum
{
	STORE_ON_MODIFIED,
	STORE_ON_EXCLUSIVE,
	STORE_ON_SHARED,
	STORE_ON_INVALID,
	LOAD_FROM_MODIFIED,
	LOAD_FROM_EXCLUSIVE,
	LOAD_FROM_SHARED,
	LOAD_FROM_INVALID,
} mesi_type_t;

const char* mesi_type_des[] = {
    "STORE_ON_MODIFIED", "STORE_ON_EXCLUSIVE", "STORE_ON_SHARED",
    "STORE_ON_INVALID",  "LOAD_FROM_MODIFIED", "LOAD_FROM_EXCLUSIVE",
    "LOAD_FROM_SHARED",  "LOAD_FROM_INVALID",
};

volatile char* shared_data         = nullptr;
volatile char** shared_iter        = nullptr;
volatile uint64_t shared_data_size = _32KB;

void __attribute__( ( optimize( "0" ) ) )
flusher_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	char** iter = (char**)shared_iter;
	for( uint64_t i = 0; i < shared_data_size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		_mm_sfence();
		_mm_clflush( (void*)iter );
		_mm_sfence();
		iter = ( (char**)*iter );
	}
}

void __attribute__( ( optimize( "0" ) ) )
writer_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	pc.start();
	uint64_t start = rdtsc();
	char** iter    = (char**)shared_iter;
	for( uint64_t i = 0; i < shared_data_size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		iter          = ( (char**)*iter );
		*( iter + 1 ) = (char*)1;
	}
	uint64_t end = rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back(end - start);
}

void __attribute__( ( optimize( "0" ) ) )
reader_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	pc.start();
	uint64_t start = rdtsc();
	// Pointer-chase through linked list
	for( uint64_t i = 0; i < shared_data_size / CACHELINE_SIZE; i++ )
	{
		// Unroll loop partially to reduce loop overhead
		shared_iter = ( (volatile char**)*shared_iter );
	}
	uint64_t end = rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back(end - start);
}

void reader( PAPILLCounter& pc ) { reader_( pc, _32KB, 64 ); }

void writer( PAPILLCounter& pc ) { writer_( pc, _32KB, 64 ); }

void flusher( PAPILLCounter& pc ) { flusher_( pc, _32KB, 64 ); }

void setup( uint64_t size, uint64_t stride = 64 )
{
	shared_data = (char*)malloc( size * sizeof( char ) );

	volatile char** head = (volatile char**)shared_data;
	shared_iter          = head;

	std::vector<char*> rndarray;
	for( uint64_t i = 0; i < size; i += stride )
	{
		rndarray.push_back( (char*)&shared_data[i] );
	}

	myshuffle<char*>( rndarray );

	for( uint64_t i = 0; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		*shared_iter = *(volatile char**)&rndarray[i / stride];

		shared_iter += ( stride / sizeof( shared_iter ) );
	}

	// Loop back end of linked list to the head
	*shared_iter = (char*)head;
}

void init_state( std::vector<Sched>& vec, uint64_t cc_state, int core_a,
                 int core_b )
{
	switch( cc_state )
	{
		case M_STATE:
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			break;
		case E_STATE:
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( flusher )>{ flusher },
			                      {} } );
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      {} } );
			break;
		case S_STATE:
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      {} } );
			vec.push_back( Sched{ core_b /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      {} } );
			break;
		case I_STATE:
			vec.push_back( Sched{ core_b /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			break;
		case F_STATE:
			vec.push_back( Sched{ core_b /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      {} } );
			break;
		default: break;
	}
}

void run_test( mesi_type_t t )
{
	CounterBenchmark<PAPILLCounter> cbench;
	std::vector<Sched> vec;
	int core_a = 1, core_b = 2, core_c = 1;
    std::cout << mesi_type_des[t] << std::endl;
	switch( t )
	{
		case STORE_ON_MODIFIED:
			init_state( vec, M_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case STORE_ON_EXCLUSIVE:
			init_state( vec, E_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case STORE_ON_SHARED:
			init_state( vec, S_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case STORE_ON_INVALID:
			init_state( vec, I_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( writer )>{ writer },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case LOAD_FROM_MODIFIED:
			init_state( vec, M_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case LOAD_FROM_EXCLUSIVE:
			init_state( vec, E_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case LOAD_FROM_SHARED:
			init_state( vec, S_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		case LOAD_FROM_INVALID:
			init_state( vec, I_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c /* core id */,
			                      std::function<decltype( reader )>{ reader },
			                      { "PAPI_TOT_INS", "PAPI_TOT_CYC" } } );
			break;
		default: break;
	}

	//cbench.counters_with_priority_schedule<std::vector<std::string>>( vec );
    
	auto counters = cbench.counters_with_priority_schedule<std::vector<std::string>>( vec );
    for( auto& cnt: counters )
    {
        if (cnt.cset.size() == 0)
            continue;

        cnt.stats();
    }
    
    
}

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

	setup( shared_data_size );

    run_test( LOAD_FROM_MODIFIED );
	run_test( LOAD_FROM_SHARED );
	run_test( LOAD_FROM_INVALID );
	run_test( LOAD_FROM_MODIFIED );
	run_test( LOAD_FROM_SHARED );
	run_test( LOAD_FROM_INVALID );
	run_test( LOAD_FROM_MODIFIED );
	run_test( LOAD_FROM_SHARED );
	run_test( LOAD_FROM_INVALID );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
