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
#include <iostream>
#include <shared_mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "constants.h"

#ifdef __FreeBSD__
#	define CPUSET_T cpuset_t
#else
#	define CPUSET_T cpu_set_t
#endif

namespace pcnt
{
void pin_self_to_core( int core_id );
uint64_t rdtsc();

template<typename EventTyp> struct Schedule
{
	int core_id;
	std::function<void( void )> benchmark;
	EventTyp events;

	Schedule( int core_id, std::function<void( void )> benchmark,
	          EventTyp events )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	{
	}
};

template<typename CntTyp> struct CounterBenchmark
{
	std::shared_mutex mtx;
	std::condition_variable_any cv;
	unsigned int benchmark_cores;
	std::vector<std::thread> threads;

	CounterBenchmark()
	    : mtx()
	    , cv()
	    , benchmark_cores( default_phys_core_count )
	    , threads()

	{
		pin_self_to_core( 0 );
	}

	void set_core_num( unsigned int benchmark_cores )
	{
		this->benchmark_cores = benchmark_cores;
	};

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
	                        int core_idx, std::function<void( void )> benchmark,
	                        bool warmup = true )
	{
		pin_to_core( th_idx, core_idx );
		counter.add( events );
		counter.start();

		if(warmup)
			benchmark();

		std::shared_lock lck( this->mtx );
		this->cv.wait( lck );

		uint64_t start = rdtsc();

		benchmark();

		uint64_t end = rdtsc();

		counter.read();
		counter.set_cycles_measured( end - start );
	}

	template<typename EventTyp>
	void counters_on_cores( EventTyp& events,
	                        std::function<void( void )> benchmark )
	{
		std::vector<CntTyp> counters{ this->benchmark_cores };
		for( unsigned int i = 0; i < this->benchmark_cores; ++i )
		{
			this->threads.push_back(
			    std::thread( [this, &counters, i, &events, benchmark] {
				    this->counter_thread_fn<EventTyp>( counters[i], events, i,
				                                       i + 1, benchmark );
			    } ) );
		}

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		this->cv.notify_all();

		for( auto& th: this->threads )
			th.join();

		for( auto& cnt: counters )
			cnt.stats();
	}

	template<typename EventTyp>
	void counters_with_schedule( std::vector<Schedule<EventTyp>>& svec )
	{
		std::vector<CntTyp> counters{ this->benchmark_cores };
		for( int i = 0; i < svec.size(); ++i )
		{
			this->threads.push_back( std::thread( [this, &counters, &svec, i] {
				this->counter_thread_fn<EventTyp>( counters[i], svec[i].events,
				                                   i, svec[i].core_id,
				                                   svec[i].benchmark );
			} ) );
		}

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		this->cv.notify_all();

		for( auto& th: this->threads )
			th.join();

		for( int i = 0; i < svec.size(); ++i )
		{
			std::cout << "Core " << svec[i].core_id << ":" << std::endl;
			counters[i].stats();
		}
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
