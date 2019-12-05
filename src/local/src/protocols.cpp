#include <err.h>
#include <immintrin.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
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

int core_src, core_node0, core_node1, core_socket0, core_socket1, core_global0,
    core_global1;

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

typedef enum
{
	LOCAL,  // Same core
	NODE,   // Same Node
	SOCKET, // Same Socket
	GLOBAL  // Across Sockets
} core_placement_t;

const char* mesi_type_des[] = {
    "STORE_ON_MODIFIED", "STORE_ON_EXCLUSIVE", "STORE_ON_SHARED",
    "STORE_ON_INVALID",  "LOAD_FROM_MODIFIED", "LOAD_FROM_EXCLUSIVE",
    "LOAD_FROM_SHARED",  "LOAD_FROM_INVALID",
};

const char* core_placement_des[] = { "LOCAL", "NODE", "SOCKET", "GLOBAL" };

volatile char* shared_data  = nullptr;
volatile char** shared_iter = nullptr;
volatile uint64_t shared_data_size;
volatile uint64_t cache_line_size;
volatile uint64_t cache_size;

void __attribute__( ( optimize( "0" ) ) )
flusher_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	for( uint64_t i = 0; i < shared_data_size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		_mm_mfence();
		_mm_clflush( (void*)&shared_data[i] );
		_mm_mfence();
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
	pc.vec_cycles_measured.push_back( end - start );
}

void __attribute__( ( optimize( "0" ) ) )
reader_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
	pc.start();
	uint64_t start = rdtsc();
	// Pointer-chase through linked list
	for( uint64_t i = 0; i < shared_data_size / cache_line_size; i++ )
	{
		// Unroll loop partially to reduce loop overhead
		shared_iter = ( (volatile char**)*shared_iter );
	}
	uint64_t end = rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back( end - start );
}

void reader( PAPILLCounter& pc )
{
	reader_( pc, shared_data_size, cache_line_size );
}

void writer( PAPILLCounter& pc )
{
	writer_( pc, shared_data_size, cache_line_size );
}

void flusher( PAPILLCounter& pc )
{
	flusher_( pc, shared_data_size, cache_line_size );
}

void init_params() {}

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
			vec.push_back( Sched{ core_a /* core id */,
			                      std::function<decltype( flusher )>{ flusher },
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

void run_test( mesi_type_t t, core_placement_t c = NODE )
{
	CounterBenchmark<PAPILLCounter> cbench;
	std::vector<Sched> vec;
	int core_a = core_src, core_b, core_c;
	switch( c )
	{
		case LOCAL:
			core_b = core_node0;
			core_c = core_src;
			break;
		case NODE:
			core_b = core_node0;
			core_c = core_node1;
			break;
		case SOCKET:
			core_b = core_socket0;
			core_c = core_socket1;
			break;
		case GLOBAL:
			core_b = core_global0;
			core_c = core_global1;
			break;
		default: break;
	}
	std::ofstream ofs( "dump.dat", std::ofstream::out | std::ofstream::app );
	ofs << "TEST RUN" << std::endl;
	ofs << mesi_type_des[t] << std::endl;
	ofs << "Core setting: " << core_placement_des[c] << " " << core_a << " "
	    << core_b << " " << core_c << std::endl;

	ofs.close();

	vec.push_back( Sched{ core_a /* core id */,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} } );
	vec.push_back( Sched{ core_b /* core id */,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} } );
	vec.push_back( Sched{ core_c /* core id */,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} } );
	switch( t )
	{
		case STORE_ON_MODIFIED:
			init_state( vec, M_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( writer )>{ writer },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case STORE_ON_EXCLUSIVE:
			init_state( vec, E_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( writer )>{ writer },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case STORE_ON_SHARED:
			init_state( vec, S_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( writer )>{ writer },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case STORE_ON_INVALID:
			init_state( vec, I_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( writer )>{ writer },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case LOAD_FROM_MODIFIED:
			init_state( vec, M_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( reader )>{ reader },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case LOAD_FROM_EXCLUSIVE:
			init_state( vec, E_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( reader )>{ reader },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case LOAD_FROM_SHARED:
			init_state( vec, S_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( reader )>{ reader },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		case LOAD_FROM_INVALID:
			init_state( vec, I_STATE, core_a, core_b );
			vec.push_back(
			    Sched{ core_c /* core id */,
			           std::function<decltype( reader )>{ reader },
			           { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
			             "perf::L1-DCACHE-LOADS" } } );
			break;
		default: break;
	}

	auto counters
	    = cbench.counters_with_priority_schedule<std::vector<std::string>>(
	        vec );
	for( auto& cnt: counters )
	{
		if( cnt.cset.size() == 0 )
			continue;

		cnt.stats();
	}
}
void parse_cfg()
{
	std::ifstream infile( "arch.cfg" );
	if( !infile )
	{
		std::cout << "Could not find file" << std::endl;
		exit( 1 );
	}
	std::cout << "Reading from the file" << std::endl;

	std::string rline;
	getline( infile, rline, '\n' );
	shared_data_size = std::stoll( rline );
	getline( infile, rline, '\n' );
	cache_size = std::stoll( rline );
	getline( infile, rline, '\n' );
	cache_line_size = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_src = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_node0 = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_node1 = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_socket0 = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_socket1 = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_global0 = std::stoi( rline );
	getline( infile, rline, '\n' );
	core_global1 = std::stoi( rline );
	infile.close();
}

int main( int argc, char* argv[] )
{
	/*
	std::cout
	    << ">>>> TEST: force caches into M/E/S/I/F and measure performance"
	    << std::endl;
	    */
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC

	parse_cfg();

	setup( shared_data_size );

	for( int j = 0; j < 4; j++ )
	{
		for( int i = 0; i < 1000; i++ )
		{
			run_test( LOAD_FROM_MODIFIED, static_cast<core_placement_t>( j ) );
			run_test( LOAD_FROM_SHARED, static_cast<core_placement_t>( j ) );
			run_test( LOAD_FROM_INVALID, static_cast<core_placement_t>( j ) );
		}
	}

	free( (void*)shared_data );

#endif // !WITH_PAPI_LL
	// std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
