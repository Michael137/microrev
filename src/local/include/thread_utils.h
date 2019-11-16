#ifndef THREAD_UTILS_H_IN
#define THREAD_UTILS_H_IN

#include <pthread.h>
#include <pthread_np.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <shared_mutex>
#include <thread>

#ifdef __FreeBSD__
#	define CPUSET cpuset_t
#else
#	define CPUSET cpu_set_t
#endif

namespace pcnt
{
void pin_self_to_core( int core_id )
{
	// Master thread on core 0
	CPUSET cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( core_id, &cpuset );

	pthread_setaffinity_np( pthread_self(), sizeof( CPUSET ), &cpuset );
}

template<typename CntTyp> struct CounterBenchmark
{
	std::function<void( void )> benchmark;
	std::shared_mutex mtx;
	std::condition_variable_any cv;
	unsigned int num_cores;
	std::vector<std::thread> threads;

	CounterBenchmark( std::function<void( void )> benchmark )
	    : benchmark( benchmark )
	    , mtx()
	    , cv()
	    , num_cores( std::thread::hardware_concurrency() )
	    , threads( this->num_cores )
	{
		pin_self_to_core( 0 );
	}

	void pin_to_core( int core_id )
	{
		CPUSET cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET( core_id, &cpuset );

		pthread_setaffinity_np( this->threads[core_id].native_handle(),
		                        sizeof( CPUSET ), &cpuset );
	}

	void counter_thread_fn( CntTyp& counter )
	{
		counter.start();

		std::shared_lock lck( this->mtx );
		this->cv.wait( lck );

		this->benchmark();

		counter.read();
	}

	template<typename EventTyp>
	void schedule_bench_with_counters( EventTyp events )
	{
		unsigned int benchmark_threads
		    = this->num_cores - 1; // TODO: should be ... - 1

		std::vector<CntTyp> counters{benchmark_threads};
		for( unsigned int i = 0; i < benchmark_threads; ++i )
		{
			pin_to_core( i + 1 );

			counters[i].add( events );
			this->threads[i] = std::thread( [this, &counters, i] {
				this->counter_thread_fn( counters[i] );
			} );
		}

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		this->cv.notify_all();

		for( auto& th: this->threads )
			if( th.joinable() )
				th.join();

		for( auto& cnt: counters )
			cnt.stats();
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
