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

template<typename EventTyp, typename CntType> struct Schedule
{
   private:
	using BenchTyp = std::function<void( CntType& )>;

   public:
	int core_id;
	BenchTyp benchmark;
	EventTyp events;

	Schedule( int core_id, BenchTyp benchmark, EventTyp events )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	{
	}
};

template<typename CntTyp> struct CounterBenchmark
{
   private:
	using BenchTyp = std::function<void( CntTyp& )>;

   public:
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
	                        int core_id, BenchTyp benchmark, int warmup = 5,
	                        bool sync = true )
	{
		uint64_t start, end;
		counter.core_id = core_id;

		if( sync )
		{
			std::unique_lock<std::mutex> lck( this->mtx );
			this->cv.wait( lck );
		}

		counter.add( events );

		for( int i = 0; i < warmup; ++i )
			benchmark( counter );

		counter.reset();

		benchmark( counter );

		counter.end();
	}

	template<typename EventTyp>
	void counters_with_schedule( std::vector<Schedule<EventTyp, CntTyp>>& svec )
	{
		std::vector<CntTyp> counters{ svec.size() };
		for( int i = 0; i < svec.size(); ++i )
		{
			this->threads.push_back( std::thread( [this, &counters, &svec, i] {
				this->counter_thread_fn<EventTyp>( counters[i], svec[i].events,
				                                   i, svec[i].core_id,
				                                   svec[i].benchmark );
			} ) );
			pin_to_core( i, svec[i].core_id );
		}

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		this->cv.notify_all();

		for( auto& th: this->threads )
			th.join();

		for( auto& cnt: counters )
		{
			cnt.stats();
		}
	}

	template<typename EventTyp>
	std::vector<CntTyp> counters_with_priority_schedule(
	    std::vector<Schedule<EventTyp, CntTyp>>& svec )
	{
		std::vector<CntTyp> counters{ svec.size() };
		for( int i = 0; i < svec.size(); ++i )
		{
			std::packaged_task<void()> task( [this, &counters, &svec, i] {
				this->counter_thread_fn<EventTyp>(
				    counters[i], svec[i].events, 0 /* thread id */,
				    svec[i].core_id, svec[i].benchmark, 0 /* warmup */,
				    false /* sync */ );
			} );
			auto fut = task.get_future();

			this->threads.push_back( std::thread( std::move( task ) ) );

			auto status = fut.wait_for( std::chrono::seconds( 30 ) );

			if( status == std::future_status::ready )
			{
				this->threads[0].join();
				this->threads.erase( this->threads.begin() );
				continue;
			}
			else
			{
				std::cout << "Serial benchmark timelimit reached." << std::endl;
				exit( EXIT_FAILURE );
			}
		}

        return counters;

		for( auto& cnt: counters )
		{
			cnt.stats();
		}
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
