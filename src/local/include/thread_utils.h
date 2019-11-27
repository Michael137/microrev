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
#include <future>
#include <iostream>
#include <mutex>
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
	bool collect;

	Schedule( int core_id, std::function<void( void )> benchmark,
	          EventTyp events )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	    , collect( true )
	{
	}

	Schedule( int core_id, std::function<void( void )> benchmark,
	          EventTyp events, bool collect )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	    , collect( collect )
	{
	}
};

template<typename CntTyp> struct CounterBenchmark
{
	std::mutex mtx;
	std::condition_variable cv;
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

	CPUSET_T pin_to_core( int th_idx, int core_id )
	{
		CPUSET_T cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET( core_id, &cpuset );

		pthread_setaffinity_np( this->threads[th_idx].native_handle(),
		                        sizeof( CPUSET_T ), &cpuset );

		return cpuset;
	}

	template<typename EventTyp>
	void counter_thread_fn( CntTyp& counter, EventTyp& events, int th_id,
	                        int core_id, std::function<void( void )> benchmark,
	                        int warmup = 3, bool sync = true )
	{
		bool collect    = counter.collect;
		counter.core_id = core_id;
		pin_to_core( th_id, core_id );

		if( collect )
		{
			counter.add( events );
			counter.start();
		}

		for(int i = 0; i < warmup; ++i)
			benchmark();

		if( sync )
		{
			std::unique_lock<std::mutex> lck( this->mtx );
			this->cv.wait( lck );
		}

		uint64_t start = rdtsc();

		benchmark();

		uint64_t end = rdtsc();

		if( collect )
		{
			counter.read();
			counter.cycles_measured = end - start;
		}
	}

	template<typename EventTyp>
	void counters_with_schedule( std::vector<Schedule<EventTyp>>& svec )
	{
		std::vector<CntTyp> counters{ this->benchmark_cores - 1 };
		for( int i = 0; i < svec.size(); ++i )
		{
			counters[i].collect = svec[i].collect;
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

		for( auto& cnt: counters )
		{
			if( cnt.collect )
			{
				cnt.stats();
			}
		}
	}

	template<typename EventTyp>
	void
	counters_with_priority_schedule( std::vector<Schedule<EventTyp>>& svec )
	{
		std::vector<CntTyp> counters{ this->benchmark_cores - 1 };
		for( int i = 0; i < svec.size(); ++i )
		{
			counters[i].collect = svec[i].collect;
			std::packaged_task<void()> task( [this, &counters, &svec, i] {
				this->counter_thread_fn<EventTyp>(
				    counters[i], svec[i].events, 0 /* thread id */,
				    svec[i].core_id, svec[i].benchmark, 0 /* warmup */, false /* sync */ );
			} );
			auto fut = task.get_future();

			this->threads.push_back( std::thread( std::move( task ) ) );

			auto status = fut.wait_for( std::chrono::seconds( 5 ) );

			if( status == std::future_status::ready )
			{
				this->threads[0].join();
				this->threads.erase( this->threads.begin() );
				continue;
			}
		}

		for( auto& cnt: counters )
		{
			if( cnt.collect )
			{
				cnt.stats();
			}
		}
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
