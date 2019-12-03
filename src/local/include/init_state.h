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
#include "constant.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif


void __attribute__( ( optimize( "0" ) ) )
flusher_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
    char** iter = (char**)shared_iter;
	for( uint64_t i = 0; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
        _mm_sfence();
        _mm_clflush((void*)iter);
        _mm_sfence();
        iter = ( (char**)*iter );
	}
}

void __attribute__( ( optimize( "0" ) ) )
writer_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{
    char** iter = (char**)shared_iter;
	for( uint64_t i = 0; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
        iter = ( (char**)*iter );
        *(iter + 1) = (char*) 1;
	}
}

void __attribute__( ( optimize( "0" ) ) )
reader_( PAPILLCounter& pc, uint64_t size, uint64_t stride = 64 )
{

	pc.start();
	uint64_t start = rdtsc();
	// Pointer-chase through linked list
	for( uint64_t i = 0; i < CACHE_SIZE/CACHELINE_SIZE; i++)
	{
		// Unroll loop partially to reduce loop overhead
		shared_iter = ( (volatile char**)*shared_iter );
	}
	uint64_t end = rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

void reader( PAPILLCounter& pc ) { reader_( pc, _32KB, 64 ); }

void writer( PAPILLCounter& pc ) { writer_( pc, _32KB, 64 ); }

void flusher( PAPILLCounter& pc ) { flusher_( pc, _32KB, 64 ); }

void init_state(cc_state) {
    std::vector<Sched> vec;
    switch(cc_state) {
        case M_STATE: 
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( writer )>{ writer },
	                      {} });
            break;
        case E_STATE:
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( writer )>{ writer },
	                      {} });
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} });
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( reader )>{ reader },
	                      {} });
            break;
        case S_STATE:
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( writer )>{ writer },
	                      {} });
	        vec.push_back(Sched{ 2 /* core id */,
	                      std::function<decltype( reader )>{ reader },
	                      {} });
            break;
        case I_STATE:
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( flusher )>{ flusher },
	                      {} });
            break;
        case F_STATE:
	        vec.push_back(Sched{ 2 /* core id */,
	                      std::function<decltype( writer )>{ writer },
	                      {} });
	        vec.push_back(Sched{ 1 /* core id */,
	                      std::function<decltype( reader )>{ reader },
	                      {} });
            break;
        default:
            break;
    }

	cbench.counters_with_priority_schedule<std::vector<std::string>>( vec );
}
