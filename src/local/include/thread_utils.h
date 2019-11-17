#ifndef THREAD_UTILS_H_IN
#define THREAD_UTILS_H_IN

#include <pthread.h>

#ifdef __FreeBSD__
#	include <pthread_np.h>
#endif

#include <sched.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <shared_mutex>
#include <thread>

#ifdef __FreeBSD__
#	define CPUSET_T cpuset_t
#else
#	define CPUSET_T cpu_set_t
#endif

namespace pcnt
{
void pin_self_to_core( int core_id )
{
	// Master thread on core 0
	CPUSET_T cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( core_id, &cpuset );

	pthread_setaffinity_np( pthread_self(), sizeof( CPUSET_T ), &cpuset );
}

template<typename CntTyp> struct CounterBenchmark
{
	std::function<void( void )> benchmark;
	std::shared_mutex mtx;
	std::condition_variable_any cv;
	unsigned int benchmark_cores;
	std::vector<std::thread> threads;

	CounterBenchmark( std::function<void( void )> benchmark )
	    : benchmark( benchmark )
	    , mtx()
	    , cv()
	    , benchmark_cores( std::thread::hardware_concurrency() - 1 )
	    , threads()
	{
		pin_self_to_core( 0 );
	}

	void pin_to_core( int th_idx, int core_id )
	{
		CPUSET_T cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET( core_id, &cpuset );

		pthread_setaffinity_np( this->threads[th_idx].native_handle(),
		                        sizeof( CPUSET_T ), &cpuset );
	}

	template<typename EventTyp>
	void counter_thread_fn( CntTyp& counter, EventTyp& events, int th_idx,
	                        int core_idx )
	{
		pin_to_core( th_idx, core_idx );
		counter.add( events );
		counter.start();

		std::shared_lock lck( this->mtx );
		this->cv.wait( lck );

		this->benchmark();

		counter.read();
	}

	template<typename EventTyp>
	void schedule_bench_with_counters( EventTyp& events )
	{
		std::vector<CntTyp> counters{ this->benchmark_cores };
		for( unsigned int i = 0; i < this->benchmark_cores; ++i )
		{
			this->threads.push_back(
			    std::thread( [this, &counters, i, &events] {
				    this->counter_thread_fn<EventTyp>( counters[i], events, i,
				                                       i + 1 );
			    } ) );
		}

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		this->cv.notify_all();

		for( auto& th: this->threads )
			th.join();

		for( auto& cnt: counters )
			cnt.stats();
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
