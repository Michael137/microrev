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

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

#define M_STATE 0
#define E_STATE 1
#define S_STATE 2
#define I_STATE 3
#define O_STATE 4
#define F_STATE 5

#define FIVE( a ) a a a a a
#define TEN( a ) FIVE( a ) FIVE( a )
#define FIFTY( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a ) TEN( a )
#define HUNDRED( a ) FIFTY( a ) FIFTY( a )
#define R_512( a ) HUNDRED( a ) HUNDRED( a ) HUNDRED( a ) HUNDRED( a ) HUNDRED( a ) TEN( a ) a a 
#define R_64( a ) FIFTY( a ) FIVE(a) FIVE(a) a a a a
#define R_128( a ) R_64(a) R_64(a)
#define R_256( a ) HUNDRED( a ) HUNDRED( a ) FIFTY( a ) FIVE( a ) a
#define R_1024( a ) R_512(a) R_512(a)
#define R_2048( a ) R_1024(a) R_1024(a)
#define R_4096( a ) R_2048(a) R_2048(a)
#define R_8192( a ) R_4096(a) R_4096(a)
#define R_16384( a ) R_4096(a) R_4096(a) R_4096(a) R_4096(a)
#define R_32768( a ) R_16384( a ) R_16384( a ) 
#define R_65536( a ) R_16384( a ) R_16384( a ) R_16384( a ) R_16384( a )
#define R_131072( a ) R_65536(a) R_65536(a)
#define R_262144( a ) R_65536(a) R_65536(a) R_65536(a) R_65536(a)
#define R_524288( a ) R_262144( a ) R_262144( a )
#define R_1048576( a ) R_262144( a ) R_262144( a ) R_262144( a ) R_262144( a )
#define R_2097152( a ) R_1048576( a ) R_1048576( a )
#define WRITE_BLOCK( li, ad, tmp)       \
do                                      \
{                                       \
    *(char **)( (char *)li + ad ) = (tmp + ad);            \
    *(char **)( (char *)li + ad + 48 ) = (tmp + ad + 48);    \
    *(char **)( (char *)li + ad + 8 ) = (tmp + ad + 8);    \
    *(char **)( (char *)li + ad + 40 ) = (tmp + ad + 40);    \
    *(char **)( (char *)li + ad + 16 ) = (tmp + ad + 16);    \
    *(char **)( (char *)li + ad + 24 ) = (tmp + ad + 24);    \
    *(char **)( (char *)li + ad + 56 ) = (tmp + ad + 56);    \
} while(0);
    

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
	volatile int ready        = 0;                                     \
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
	void OPT0 cp_producer_( PAPILLCounter& pc, uint64_t size,                     \
	                     uint64_t stride = 64 )                                \
	{                                                                          \
		char** iter       = (char**)shared_data;                               \
        char* local_arr = (char*)malloc( size * sizeof( char ) );                  \
        char** local_iter = (char **)local_arr;             \
        char* tmp;                \
		uint64_t start, end;                                          \
        uint64_t blksize = _128KB;                                \
		uint64_t line_cnt = blksize / stride;                         \
		uint64_t outer_line_cnt = size / blksize;                         \
        auto addr_diff = ((char *)iter - (char *)local_iter);           \
        for( uint64_t i = 0; i < size; ++i )                               \
            local_arr[i] = 0;                               \
        set_shuffled_linked_list(local_arr, size, stride);           \
        start = rdtsc();                                                 \
        for( uint64_t i = 0; i < outer_line_cnt; i++ ) {                                 \
            while( !__sync_bool_compare_and_swap( &ready, 0, -1 ) );                \
		    pc.vec_cycles_measured.push_back( rdtsc() - start );                       \
            for( uint64_t j = 0; j < line_cnt; j++ )                               \
            {                                                                      \
                tmp = *( local_iter );                                         \
                WRITE_BLOCK( local_iter, addr_diff, tmp );                   \
                local_iter = (char**)tmp;                              \
            }                                                                      \
		    pc.vec_cycles_measured.push_back( rdtsc() - start );                       \
            __sync_bool_compare_and_swap(&ready, -1, 1);                \
		    pc.vec_cycles_measured.push_back( rdtsc() - start);                       \
        }            \
		pc.vec_cycles_measured.push_back( start );                       \
    }                              \
	void OPT0 cp_consumer_( PAPILLCounter& pc, uint64_t size,                     \
	                     uint64_t stride = 64 )                                \
	{                                                                          \
		char** iter       = (char**)shared_data;                               \
        char* local_arr = (char*)malloc( size * sizeof( char ) );                  \
        char** local_iter = (char **)local_arr;             \
        uint64_t blksize = _128KB;                                \
		uint64_t line_cnt = blksize / stride;                         \
		uint64_t outer_line_cnt = size / blksize;                         \
		uint64_t start, end;                                          \
        char* tmp;                              \
        int64_t addr_diff = ( (char *)local_iter - (char *)iter );                              \
        start = rdtsc();            \
        for( uint64_t i = 0; i < outer_line_cnt; i++ ) {                                 \
            while( !__sync_bool_compare_and_swap( &ready, 1, 2) );                \
		    pc.vec_cycles_measured.push_back( rdtsc() - start );                       \
            for( uint64_t j = 0; j < line_cnt; j++ )                               \
            {                                                                      \
                tmp = *( iter );                              \
                WRITE_BLOCK( iter, addr_diff, tmp );                   \
                iter = (char**)tmp;                                 \
            }                                                                      \
		    pc.vec_cycles_measured.push_back( rdtsc() - start );                       \
            __sync_bool_compare_and_swap( &ready, 2, 0 );                \
		    pc.vec_cycles_measured.push_back( rdtsc() - start );                       \
        }                       \
		end = rdtsc();                                                \
		pc.vec_cycles_measured.push_back( end );                       \
    }                              \
                                                                               \
	void OPT0 writer_( PAPILLCounter& pc, uint64_t size,                       \
	                   uint64_t stride = 64 )                                  \
	{                                                                          \
		char** iter       = (char**)shared_data;                               \
		pc.start();                                                            \
		uint64_t start = rdtsc();                                              \
		R_131072( iter = (char**)*iter; \
		      *(char **)( (char *)iter + 8 ) = (char *)1;                                          \
		    ) \
		uint64_t end = rdtsc();                                                \
		pc.read();                                                             \
		pc.vec_cycles_measured.push_back( end - start );                       \
	}                                                                          \
                                                                               \
	void OPT0 reader_( PAPILLCounter& pc, uint64_t size,                       \
	                   uint64_t stride = 64 )                                  \
	{                                                                          \
		char** iter       = (char**)shared_data;                               \
		pc.start();                                                            \
		uint64_t start = rdtsc();                                              \
		R_131072( iter = (char**)*iter; ) \
		uint64_t end = rdtsc();                                                \
		pc.read();                                                             \
		pc.vec_cycles_measured.push_back( end - start );                       \
	}                                                                          \
	void cp_producer( PAPILLCounter& pc )                                         \
	{                                                                          \
		cp_producer_( pc, shared_data_size, cache_line_size );                    \
	}                                                                          \
	void cp_consumer( PAPILLCounter& pc )                                         \
	{                                                                          \
		cp_consumer_( pc, shared_data_size, cache_line_size );                    \
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
                                                                               \
		for( uint64_t i = 0; i < size; i++ )                                   \
		{                                                                      \
			shared_data[i] = 0;                                                \
		}                                                                      \
                                                                               \
		volatile char** head = (volatile char**)shared_data;                   \
		shared_iter          = head;                                           \
        char *tmp = (char *)shared_data;                                       \
        set_shuffled_linked_list(tmp, size, stride);                           \
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
				vec.push_back( Sched{                                          \
				    core_a, std::function<decltype( cp_producer )>{ cp_producer },   \
				    cnt_vec, true } );                                               \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( cp_consumer )>{ cp_consumer },   \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case STORE_ON_MODIFIED:                                            \
				init_state( vec, M_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case STORE_ON_EXCLUSIVE:                                           \
				init_state( vec, E_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case STORE_ON_SHARED_OR_FORWARD:                                   \
				init_state( vec, S_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case STORE_ON_INVALID:                                             \
				init_state( vec, I_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( writer )>{ writer },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case LOAD_FROM_MODIFIED:                                           \
				init_state( vec, M_STATE, core_a, core_b );                    \
                                                                               \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec, true} );                                               \
				break;                                                         \
			case LOAD_FROM_EXCLUSIVE:                                          \
				init_state( vec, E_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case LOAD_FROM_SHARED_OR_FORWARD:                                  \
				init_state( vec, S_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case LOAD_FROM_INVALID:                                            \
				init_state( vec, I_STATE, core_a, core_b );                    \
				vec.push_back( Sched{                                          \
				    core_c, std::function<decltype( reader )>{ reader },       \
				    cnt_vec, true } );                                               \
				break;                                                         \
			case FLUSH:                                                        \
				vec.push_back(                                                 \
				    Sched{ core_a,                                             \
				           std::function<decltype( flusher )>{ flusher },      \
				           {}, true } );                                             \
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
            if(cnt.collect)                                                    \
			    cnt.stats();                                                   \
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
