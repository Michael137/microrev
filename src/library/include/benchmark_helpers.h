#ifndef BENCHMARK_HELPERS_H_IN
#define BENCHMARK_HELPERS_H_IN

#include <err.h>
#include <immintrin.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "constants.h"
#include "counter.h"
#include "counters.h"
#include "shuffle.h"
#include "thread_utils.h"

#ifdef WITH_PAPI_LL
#	include <papi.h>
#endif

#define M_STATE 0
#define E_STATE 1
#define S_STATE 2
#define I_STATE 3
#define O_STATE 4
#define F_STATE 5

uint64_t avg_no_overflow( std::vector<uint64_t> const& nums );
using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

typedef enum
{
	PRODUCER_CONSUMER,
	STORE_ON_MODIFIED,
	STORE_ON_EXCLUSIVE,
	STORE_ON_SHARED_OR_FORWARD,
	STORE_ON_INVALID,
	LOAD_FROM_MODIFIED,
	LOAD_FROM_EXCLUSIVE,
	LOAD_FROM_SHARED_OR_FORWARD,
	LOAD_FROM_INVALID,
	FLUSH,
} mesi_type_t;

typedef enum
{
	LOCAL,  // Same core
	SOCKET, // Same Socket
	GLOBAL  // Across Sockets
} core_placement_t;

static const char* mesi_type_des[] = {
    "PRODUCER_CONSUMER",   "STORE_ON_MODIFIED",
    "STORE_ON_EXCLUSIVE",  "STORE_ON_SHARED_OR_FORWARD",
    "STORE_ON_INVALID",    "LOAD_FROM_MODIFIED",
    "LOAD_FROM_EXCLUSIVE", "LOAD_FROM_SHARED_OR_FORWARD",
    "LOAD_FROM_INVALID",   "FLUSH",
};

static const char* core_placement_des[] = { "LOCAL", "SOCKET", "GLOBAL" };

#ifdef __llvm__
#	define DEMOTER_DEF()                                                      \
		void demoter_( PAPILLCounter& pc, uint64_t size,                       \
		               uint64_t stride = 64 )                                  \
		{                                                                      \
			for( uint64_t i = 0; i < shared_data_size; i += stride )           \
			{                                                                  \
				_mm_mfence();                                                  \
				_cldemote( (void*)&shared_data[i] );                           \
				_mm_mfence();                                                  \
			}                                                                  \
		}
#else
#	define DEMOTER_DEF()                                                      \
		void demoter_( PAPILLCounter& pc, uint64_t size,                       \
		               uint64_t stride = 64 )                                  \
		{                                                                      \
		}
#endif

#define BENCHMARK_INIT()                                                       \
	int core_src, core_socket0, core_socket1, core_global0, core_global1;      \
                                                                               \
	volatile char* shared_data  = nullptr;                                     \
	volatile int* wrflag        = nullptr;                                     \
	volatile char** shared_iter = nullptr;                                     \
	volatile uint64_t shared_data_size;                                        \
	volatile uint64_t cache_line_size;                                         \
	volatile uint64_t cache_size;                                              \
	volatile uint64_t producer_start_time;                                     \
                                                                               \
	void OPT0 flusher_( PAPILLCounter& pc, uint64_t size,                      \
	                    uint64_t stride = 64 )                                 \
	{                                                                          \
		for( uint64_t i = 0; i < shared_data_size; i++ )                       \
		{                                                                      \
			_mm_mfence();                                                      \
			_mm_clflush( (void*)&shared_data[i] );                             \
			_mm_mfence();                                                      \
		}                                                                      \
	}                                                                          \
	void OPT0 producer_( PAPILLCounter& pc, uint64_t size,                     \
	                     uint64_t stride = 64 )                                \
	{                                                                          \
		char** iter       = (char**)shared_iter;                               \
		uint64_t line_cnt = shared_data_size / stride;                         \
		pc.start();                                                            \
		uint64_t start = rdtsc();                                              \
		for( uint64_t i = 0; i < line_cnt; i++ )                               \
		{                                                                      \
			*( iter + 1 ) = (char*)1;                                          \
			iter          = ( (char**)*iter );                                 \
			__sync_fetch_and_add( &wrflag[i], -1 );                            \
		}                                                                      \
		pc.read();                                                             \
		pc.vec_cycles_measured.push_back( start );                             \
	}                                                                          \
	void OPT0 consumer_( PAPILLCounter& pc, uint64_t size,                     \
	                     uint64_t stride = 64 )                                \
	{                                                                          \
		char** iter       = (char**)shared_iter;                               \
		uint64_t line_cnt = shared_data_size / stride;                         \
		pc.start();                                                            \
		for( uint64_t i = 0; i < line_cnt; i++ )                               \
		{                                                                      \
			while( !__sync_lock_test_and_set( &wrflag[i], 1 ) )                \
			{                                                                  \
				_mm_pause();                                                   \
			};                                                                 \
			iter = ( (char**)*iter );                                          \
		}                                                                      \
		uint64_t end = rdtsc();                                                \
		pc.read();                                                             \
		pc.vec_cycles_measured.push_back( end );                               \
	}                                                                          \
                                                                               \
	void OPT0 writer_( PAPILLCounter& pc, uint64_t size,                       \
	                   uint64_t stride = 64 )                                  \
	{                                                                          \
		char** iter       = (char**)shared_iter;                               \
		uint64_t line_cnt = shared_data_size / stride;                         \
		pc.start();                                                            \
		uint64_t start = rdtsc();                                              \
		for( uint64_t i = 0; i < line_cnt; i++ )                               \
		{                                                                      \
			iter          = ( (char**)*iter );                                 \
			*( iter + 1 ) = (char*)1;                                          \
		}                                                                      \
		uint64_t end = rdtsc();                                                \
		pc.read();                                                             \
		pc.vec_cycles_measured.push_back( end - start );                       \
	}                                                                          \
                                                                               \
	void OPT0 reader_( PAPILLCounter& pc, uint64_t size,                       \
	                   uint64_t stride = 64 )                                  \
	{                                                                          \
		char** iter       = (char**)shared_iter;                               \
		uint64_t line_cnt = shared_data_size / stride;                         \
		pc.start();                                                            \
		uint64_t start = rdtsc();                                              \
		for( uint64_t i = 0; i < line_cnt; i++ )                               \
		{                                                                      \
			iter = ( (char**)*iter );                                          \
		}                                                                      \
		uint64_t end = rdtsc();                                                \
		pc.read();                                                             \
		pc.vec_cycles_measured.push_back( end - start );                       \
	}                                                                          \
	void producer( PAPILLCounter& pc )                                         \
	{                                                                          \
		producer_( pc, shared_data_size, cache_line_size );                    \
	}                                                                          \
	void consumer( PAPILLCounter& pc )                                         \
	{                                                                          \
		consumer_( pc, shared_data_size, cache_line_size );                    \
	}                                                                          \
                                                                               \
	void reader( PAPILLCounter& pc )                                           \
	{                                                                          \
		reader_( pc, shared_data_size, cache_line_size );                      \
	}                                                                          \
                                                                               \
	void writer( PAPILLCounter& pc )                                           \
	{                                                                          \
		writer_( pc, shared_data_size, cache_line_size );                      \
	}                                                                          \
                                                                               \
	void flusher( PAPILLCounter& pc )                                          \
	{                                                                          \
		flusher_( pc, shared_data_size, cache_line_size );                     \
	}                                                                          \
                                                                               \
	DEMOTER_DEF()                                                              \
	void demoter( PAPILLCounter& pc )                                          \
	{                                                                          \
		demoter_( pc, shared_data_size, cache_line_size );                     \
	}                                                                          \
	void pr_co_setup( uint64_t size, uint64_t stride = 64 )                    \
	{                                                                          \
		shared_data = (char*)malloc( size * sizeof( char ) );                  \
		wrflag      = (int*)malloc( size / stride * sizeof( int ) );           \
                                                                               \
		for( uint64_t i = 0; i < size / stride; i++ )                          \
		{                                                                      \
			wrflag[i] = 1;                                                     \
		}                                                                      \
		for( uint64_t i = 0; i < size; i++ )                                   \
		{                                                                      \
			shared_data[i] = 0;                                                \
		}                                                                      \
                                                                               \
		volatile char** head = (volatile char**)shared_data;                   \
		shared_iter          = head;                                           \
                                                                               \
		std::vector<char*> rndarray;                                           \
		for( uint64_t i = 0; i < size; i += stride )                           \
		{                                                                      \
			rndarray.push_back( (char*)&shared_data[i] );                      \
		}                                                                      \
                                                                               \
		shuffle_array<char*>( rndarray );                                      \
                                                                               \
		for( uint64_t i = 0; i < size; i += stride )                           \
		{                                                                      \
			*shared_iter = *(volatile char**)&rndarray[i / stride];            \
                                                                               \
			shared_iter += ( stride / sizeof( shared_iter ) );                 \
		}                                                                      \
		*shared_iter = (char*)head;                                            \
	}                                                                          \
                                                                               \
	void setup( uint64_t size, uint64_t stride = 64 )                          \
	{                                                                          \
		shared_data = (char*)malloc( size * sizeof( char ) );                  \
                                                                               \
		volatile char** head = (volatile char**)shared_data;                   \
		shared_iter          = head;                                           \
                                                                               \
		std::vector<char*> rndarray;                                           \
		for( uint64_t i = 0; i < size; i += stride )                           \
		{                                                                      \
			rndarray.push_back( (char*)&shared_data[i] );                      \
		}                                                                      \
                                                                               \
		shuffle_array<char*>( rndarray );                                      \
                                                                               \
		for( uint64_t i = 0; i < size; i += stride )                           \
		{                                                                      \
			*shared_iter = *(volatile char**)&rndarray[i / stride];            \
                                                                               \
			shared_iter += ( stride / sizeof( shared_iter ) );                 \
		}                                                                      \
		*shared_iter = (char*)head;                                            \
	}                                                                          \
                                                                               \
	void init_state(                                                           \
	    std::vector<Sched>& vec, uint64_t cc_state, int core_a, int core_b,    \
	    std::vector<std::string> cnt_vec = std::vector<std::string>{} )        \
	{                                                                          \
		switch( cc_state )                                                     \
		{                                                                      \
			case M_STATE:                                                      \
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( writer )>{ writer },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case E_STATE:                                                      \
				vec.push_back(                                                 \
				    Sched{ core_a,                                             \
				           std::function<decltype( writer )>{ writer },        \
				           {} } );                                             \
				vec.push_back(                                                 \
				    Sched{ core_a,                                             \
				           std::function<decltype( flusher )>{ flusher },      \
				           {} } );                                             \
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( reader )>{ reader },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case S_STATE:                                                      \
				vec.push_back(                                                 \
				    Sched{ core_a,                                             \
				           std::function<decltype( writer )>{ writer },        \
				           {} } );                                             \
				vec.push_back(                                                 \
				    Sched{ core_a,                                             \
				           std::function<decltype( reader )>{ reader },        \
				           {} } );                                             \
				vec.push_back(                                                 \
				    Sched{ core_b,                                             \
				           std::function<decltype( reader )>{ reader },        \
				           {} } );                                             \
				break;                                                         \
			case I_STATE:                                                      \
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( writer )>{ writer },       \
				    cnt_vec } );                                               \
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( flusher )>{ flusher },     \
				    cnt_vec } );                                               \
				break;                                                         \
			case F_STATE:                                                      \
				vec.push_back(                                                 \
				    Sched{ core_b,                                             \
				           std::function<decltype( writer )>{ writer },        \
				           {} } );                                             \
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( reader )>{ reader },       \
				    cnt_vec } );                                               \
				vec.push_back(                                                 \
				    Sched{ core_b,                                             \
				           std::function<decltype( flusher )>{ flusher },      \
				           {} } );                                             \
				break;                                                         \
			default: break;                                                    \
		}                                                                      \
	}                                                                          \
                                                                               \
	void run_test( mesi_type_t t, core_placement_t c,                          \
	               std::vector<std::string> cnt_vec )                          \
	{                                                                          \
		CounterBenchmark<PAPILLCounter> cbench;                                \
		std::vector<Sched> vec;                                                \
		int core_a = core_src, core_b, core_c;                                 \
		switch( c )                                                            \
		{                                                                      \
			case LOCAL:                                                        \
				core_b = core_socket0;                                         \
				core_c = core_src;                                             \
				break;                                                         \
			case SOCKET:                                                       \
				core_b = core_socket0;                                         \
				core_c = core_socket1;                                         \
				break;                                                         \
			case GLOBAL:                                                       \
				core_b = core_global0;                                         \
				core_c = core_global1;                                         \
				break;                                                         \
			default: break;                                                    \
		}                                                                      \
		std::ofstream ofs( "dump.dat",                                         \
		                   std::ofstream::out | std::ofstream::app );          \
		ofs << "TEST RUN" << std::endl;                                        \
		ofs << "Working Set Size: " << shared_data_size << std::endl;          \
		ofs << mesi_type_des[t] << std::endl;                                  \
		ofs << "Core setting: " << core_placement_des[c] << " " << core_a      \
		    << " " << core_b << " " << core_c << std::endl;                    \
                                                                               \
		ofs.close();                                                           \
                                                                               \
		vec.push_back( Sched{                                                  \
		    core_a, std::function<decltype( flusher )>{ flusher }, {} } );     \
		vec.push_back( Sched{                                                  \
		    core_b, std::function<decltype( flusher )>{ flusher }, {} } );     \
		vec.push_back( Sched{                                                  \
		    core_c, std::function<decltype( flusher )>{ flusher }, {} } );     \
		switch( t )                                                            \
		{                                                                      \
			case PRODUCER_CONSUMER:                                            \
				vec.clear();                                                   \
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( producer )>{ producer },   \
				    cnt_vec } );                                               \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( consumer )>{ consumer },   \
				    cnt_vec } );                                               \
				break;                                                         \
			case STORE_ON_MODIFIED:                                            \
				init_state( vec, M_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case STORE_ON_EXCLUSIVE:                                           \
				init_state( vec, E_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case STORE_ON_SHARED_OR_FORWARD:                                   \
				init_state( vec, S_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case STORE_ON_INVALID:                                             \
				init_state( vec, I_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case LOAD_FROM_MODIFIED:                                           \
				init_state( vec, M_STATE, core_a, core_b );                    \
                                                                               \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case LOAD_FROM_EXCLUSIVE:                                          \
				init_state( vec, E_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case LOAD_FROM_SHARED_OR_FORWARD:                                  \
				init_state( vec, S_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case LOAD_FROM_INVALID:                                            \
				init_state( vec, I_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec } );                                               \
				break;                                                         \
			case FLUSH:                                                        \
				vec.push_back(                                                 \
				    Sched{ core_a,                                             \
				           std::function<decltype( flusher )>{ flusher },      \
				           {} } );                                             \
				break;                                                         \
			default: break;                                                    \
		}                                                                      \
                                                                               \
		std::vector<PAPILLCounter> counters;                                   \
		if( t == PRODUCER_CONSUMER )                                           \
			counters                                                           \
			    = cbench.counters_with_schedule<std::vector<std::string>>(     \
			        vec, 0 );                                                  \
		else                                                                   \
			counters = cbench.counters_with_priority_schedule<                 \
			    std::vector<std::string>>( vec );                              \
		for( auto& cnt: counters )                                             \
		{                                                                      \
			if( cnt.cset.size() == 0 )                                         \
				continue;                                                      \
                                                                               \
			cnt.stats();                                                       \
		}                                                                      \
	}                                                                          \
	void parse_cfg()                                                           \
	{                                                                          \
		std::ifstream infile( "arch.cfg" );                                    \
		if( !infile )                                                          \
		{                                                                      \
			std::cout << "Could not find file" << std::endl;                   \
			exit( 1 );                                                         \
		}                                                                      \
		std::cout << "Reading from the file" << std::endl;                     \
                                                                               \
		std::string rline;                                                     \
		getline( infile, rline, '\n' );                                        \
		shared_data_size = std::stoll( rline );                                \
		getline( infile, rline, '\n' );                                        \
		cache_size = std::stoll( rline );                                      \
		getline( infile, rline, '\n' );                                        \
		cache_line_size = std::stoi( rline );                                  \
		getline( infile, rline, '\n' );                                        \
		core_src = std::stoi( rline );                                         \
		getline( infile, rline, '\n' );                                        \
		core_socket0 = std::stoi( rline );                                     \
		getline( infile, rline, '\n' );                                        \
		core_socket1 = std::stoi( rline );                                     \
		getline( infile, rline, '\n' );                                        \
		core_global0 = std::stoi( rline );                                     \
		getline( infile, rline, '\n' );                                        \
		core_global1 = std::stoi( rline );                                     \
		infile.close();                                                        \
	}

#define INIT_ARCH_CFG( csrc, csock0, csock1, cglobal0, cglobal1, csize,        \
                       clsize, data_size )                                     \
	do                                                                         \
	{                                                                          \
		core_src         = ( csrc );                                           \
		core_socket0     = ( csock0 );                                         \
		core_socket1     = ( csock1 );                                         \
		core_global0     = ( cglobal0 );                                       \
		core_global1     = ( cglobal1 );                                       \
		cache_line_size  = ( clsize );                                         \
		cache_size       = ( csize );                                          \
		shared_data_size = ( data_size );                                      \
	} while( 0 );

#define BENCHMARK_END()                                                        \
	do                                                                         \
	{                                                                          \
		free( (void*)shared_data );                                            \
	} while( 0 );

#endif // BENCHMARK_HELPERS_H_IN
