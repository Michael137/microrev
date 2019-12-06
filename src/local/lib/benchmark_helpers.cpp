
#include <err.h>
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

#include "benchmark_helpers.h"
#include "counter.h"
#include "counters.h"
#include "shuffle.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

BENCHMARK_INIT()

const char* mesi_type_des[] = {
    "STORE_ON_MODIFIED", "STORE_ON_EXCLUSIVE", "STORE_ON_SHARED",
    "STORE_ON_INVALID",  "LOAD_FROM_MODIFIED", "LOAD_FROM_EXCLUSIVE",
    "LOAD_FROM_SHARED",  "LOAD_FROM_INVALID",  "FLUSH",
};

const char* core_placement_des[] = { "LOCAL", "SOCKET", "GLOBAL" };

static void OPT0 flusher_( pcnt::PAPILLCounter& pc, uint64_t size,
                           uint64_t stride )
{
	for( uint64_t i = 0; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		_mm_mfence();
		_mm_clflush( (void*)&shared_data[i] );
		_mm_mfence();
	}
}

static void OPT0 writer_( pcnt::PAPILLCounter& pc, uint64_t size,
                          uint64_t stride )
{
	pc.start();
	uint64_t start = pcnt::rdtsc();
	char** iter    = (char**)shared_iter;
	for( uint64_t i = 0; i < shared_data_size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		iter          = ( (char**)*iter );
		*( iter + 1 ) = (char*)1;
	}
	uint64_t end = pcnt::rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back( end - start );
}

static void OPT0 reader_( pcnt::PAPILLCounter& pc, uint64_t size,
                          uint64_t stride )
{
	pc.start();
	uint64_t start = pcnt::rdtsc();
	// Pointer-chase through linked list
	for( uint64_t i = 0; i < shared_data_size / cache_line_size; i++ )
		shared_iter = ( (volatile char**)*shared_iter );

	uint64_t end = pcnt::rdtsc();
	pc.read();
	pc.vec_cycles_measured.push_back( end - start );
}

static void parse_arch_cfg()
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
	stride = std::stoi( rline );

	getline( infile, rline, '\n' );
	core_src = std::stoi( rline );

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

void reader( pcnt::PAPILLCounter& pc )
{
	reader_( pc, shared_data_size, stride );
}

void writer( pcnt::PAPILLCounter& pc )
{
	writer_( pc, shared_data_size, stride );
}

void flusher( pcnt::PAPILLCounter& pc )
{
	flusher_( pc, shared_data_size, stride );
}

void benchmark_setup()
{
	parse_arch_cfg();

	shared_data          = (char*)malloc( shared_data_size * sizeof( char ) );
	volatile char** head = (volatile char**)shared_data;
	shared_iter          = head;

	std::vector<char*> rndarray;
	for( uint64_t i = 0; i < shared_data_size; i += stride )
		rndarray.push_back( (char*)&shared_data[i] );

	shuffle_array<char*>( rndarray );

	uint64_t i;
	for( i = 0; i < shared_data_size; i += stride )
	{
		*shared_iter = *(volatile char**)&rndarray[i / stride];
		shared_iter += ( stride / sizeof( shared_iter ) );
	}
	*shared_iter = (char*)head;
}

void set_state( std::vector<Sched>& vec, uint64_t cc_state, int core_a,
                int core_b,
                std::vector<std::string> cnt_vec = std::vector<std::string>{} )
{
	switch( cc_state )
	{
		case M_STATE:
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      cnt_vec } );
			break;
		case E_STATE:
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( flusher )>{ flusher },
			                      {} } );
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      cnt_vec } );
			break;
		case S_STATE:
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      {} } );
			vec.push_back( Sched{ core_b, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      {} } );
			vec.push_back( Sched{ core_b, // core id,
			                      std::function<decltype( flusher )>{ flusher },
			                      {} } );
			break;
		case I_STATE:

			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( flusher )>{ flusher },
			                      cnt_vec } );
			break;
		case F_STATE:
			vec.push_back( Sched{ core_b, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      cnt_vec } );
			vec.push_back( Sched{ core_b, // core id,
			                      std::function<decltype( flusher )>{ flusher },
			                      {} } );
			break;
		default: break;
	}
}

void run_test( mesi_type_t t, core_placement_t c,
               std::vector<std::string> cnt_vec )
{
	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;
	std::vector<Sched> vec;
	int core_a = core_src, core_b, core_c;
	switch( c )
	{
		case LOCAL:
			core_b = core_socket0;
			core_c = core_src;
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

	vec.push_back( Sched{ core_a, // core id,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} } );
	vec.push_back( Sched{ core_b, // core id,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} } );
	vec.push_back( Sched{ core_c, // core id,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} } );
	switch( t )
	{
		case STORE_ON_MODIFIED:
			set_state( vec, M_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      cnt_vec } );
			break;
		case STORE_ON_EXCLUSIVE:
			set_state( vec, E_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      cnt_vec } );
			break;
		case STORE_ON_SHARED:
			set_state( vec, S_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      cnt_vec } );
			break;
		case STORE_ON_INVALID:
			set_state( vec, I_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      cnt_vec } );
			break;
		case LOAD_FROM_MODIFIED:
			set_state( vec, M_STATE, core_a, core_b );

			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      cnt_vec } );
			break;
		case LOAD_FROM_EXCLUSIVE:
			set_state( vec, E_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      cnt_vec } );
			break;
		case LOAD_FROM_SHARED:
			set_state( vec, S_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      cnt_vec } );
			break;
		case LOAD_FROM_INVALID:
			set_state( vec, I_STATE, core_a, core_b );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( reader )>{ reader },
			                      cnt_vec } );
			break;
		case FLUSH:
			vec.push_back( Sched{ core_a, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      {} } );
			vec.push_back( Sched{ core_c, // core id,
			                      std::function<decltype( writer )>{ writer },
			                      cnt_vec } );
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
