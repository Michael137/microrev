#include <err.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "counter.h"
#include "counters.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

#define NUM_FLOPS 10000
#define NUM_EVENTS 1
#define NUM_THREADS                                                            \
	3 // TODO: Make dependent on number of cores/benchmark
	  // Should be num of cores - 1 (which is master thread)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;

void do_flops( int flops )
{
	float x = 0.0;
	for( int i = 0; i < flops * flops; ++i )
		x *= 0.2;
}

/*
 * pthread benchmark utils
 */
void pin_to_core( int core_id )
{
	cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( core_id, &cpuset );

	pthread_t current_thread = pthread_self();
	pthread_setaffinity_np( current_thread, sizeof( cpu_set_t ), &cpuset );
}

typedef struct
{
	pthread_t tid;
	pcnt::PAPILLCounter counter;
	int core_id;
} thread_arg;

void* thread_fn( void* arg )
{
	thread_arg* ta = static_cast<thread_arg*>( arg );
	pin_to_core( ta->core_id );
	ta->counter.start();

	pthread_mutex_lock( &mutex );
	pthread_cond_wait( &cond, &mutex );
	pthread_mutex_unlock( &mutex );

	do_flops( NUM_FLOPS );

	ta->counter.read();
	ta->counter.stats();

	pthread_exit( NULL );
}

int main( int argc, char* argv[] )
{
#ifdef __FreeBSD__
	auto FreeBSDCounters = pcnt::CounterMap["FreeBSD"];
	pcnt::PFMCounter counter( FreeBSDCounters["dcache"] );
	counter.start( counter );
	do_flops();
	counter.read();
	counter.stats();
#elif defined( WITH_PAPI_HL )
	std::vector<int> events{ PAPI_TOT_INS };
	pcnt::PAPIHLCounter pcounter( events );

	/* Start counting events */
	pcounter.start();

	do_flops( NUM_FLOPS );

	pcounter.read();
	pcounter.stats();
#elif defined( WITH_PAPI_LL )
	std::vector<int> events = { PAPI_TOT_INS, PAPI_TOT_CYC, PAPI_FP_OPS };
	pin_to_core( 0 ); // Master thread on core 0
	thread_arg* threads = static_cast<thread_arg*>(
	    calloc( NUM_THREADS, sizeof( thread_arg ) ) );
	for( int i = 0; i < NUM_THREADS; ++i )
	{
		threads[i].core_id = i + 1;
		threads[i].counter.init();
		threads[i].counter.add( events );
		pthread_create( &( threads[i].tid ), NULL, thread_fn, &threads[i] );
	}

	sleep( 1 );
	pthread_cond_broadcast( &cond );

	for( int i = 0; i < NUM_THREADS; ++i )
		pthread_join( threads[i].tid, NULL );
#endif

	return 0;
}
