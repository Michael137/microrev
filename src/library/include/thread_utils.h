#ifndef THREAD_UTILS_H_IN
#define THREAD_UTILS_H_IN

#include <pthread.h>
#ifdef __FreeBSD__
#	include <pthread_np.h>
#endif
#include <immintrin.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
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

template<typename EventTyp, typename CntTyp> struct Schedule
{
   private:
	using BenchTyp = std::function<void( CntTyp& )>;

   public:
	int core_id;
	BenchTyp benchmark;
	EventTyp events;
	std::vector<Schedule<EventTyp, CntTyp>> measurement_scheds;
	std::string label;

	Schedule( int core_id, BenchTyp benchmark, EventTyp events )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	    , measurement_scheds()
	    , label()
	{
	}

	Schedule( int core_id, EventTyp events )
	    : core_id( core_id )
	    , benchmark()
	    , events( events )
	    , measurement_scheds()
	    , label()
	{
	}

	Schedule( int core_id, BenchTyp benchmark, EventTyp events,
	          std::vector<Schedule<EventTyp, CntTyp>> const& scheds )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	    , measurement_scheds( scheds )
	    , label()
	{
	}

	Schedule( int core_id, BenchTyp benchmark, EventTyp events,
	          std::string const& label )
	    : core_id( core_id )
	    , benchmark( benchmark )
	    , events( events )
	    , measurement_scheds()
	    , label( label )
	{
	}
};

template<typename CntTyp> struct CounterBenchmark
{
   private:
	using BenchTyp = std::function<void( CntTyp& )>;

	std::mutex mtx;
	std::condition_variable cv;
	unsigned int benchmark_cores;
	std::vector<std::thread> threads;
	std::atomic<bool> pause; // Worker threads
	std::atomic<bool> ready; // Scheduler thread (always core 0)

   public:
	CounterBenchmark()
	    : mtx()
	    , cv()
	    , benchmark_cores( default_phys_core_count )
	    , threads()
	    , pause( true )
	    , ready( false )
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

		int retval
		    = pthread_setaffinity_np( this->threads[th_idx].native_handle(),
		                              sizeof( CPUSET_T ), &cpuset );

		if( retval != 0 )
		{
			std::cout << "Failed to pin thread " << th_idx << " to core "
			          << core_id << std::endl;
			exit( EXIT_FAILURE );
		}

		return cpuset;
	}

	template<typename EventTyp>
	void counter_thread_fn( CntTyp& counter, EventTyp& events, int th_id,
	                        int core_id, BenchTyp benchmark, int warmup = 5 )
	{
		// Wait for scheduler
		std::unique_lock<std::mutex> lck( this->mtx );
		auto not_paused = [this]() { return this->pause == false; };
		this->cv.wait( lck, not_paused );

		uint64_t start, end;
		counter.core_id = core_id;

		counter.add( events );

		for( int i = 0; i < warmup; ++i )
			benchmark( counter );

		counter.reset();

		benchmark( counter );

		counter.end();

		// Wake up scheduler
		this->ready = true;
		lck.unlock();
		this->cv.notify_one();
	}

	template<typename EventTyp>
	std::vector<CntTyp>
	counters_with_schedule( std::vector<Schedule<EventTyp, CntTyp>>& svec,
	                        int warmup = 5 )
	{
		std::vector<CntTyp> counters{ svec.size() };
		for( int i = 0; i < svec.size(); ++i )
		{
			counters[i].label = svec[i].label;
			this->threads.push_back( std::thread( [this, &counters, &svec, i,
			                                       warmup] {
				this->counter_thread_fn<EventTyp>( counters[i], svec[i].events,
				                                   i, svec[i].core_id,
				                                   svec[i].benchmark, warmup );
			} ) );
			pin_to_core( i, svec[i].core_id );
		}

		{
			std::lock_guard<std::mutex> lck( this->mtx );
			this->pause = false;
		}
		this->cv.notify_all();

		for( auto& th: this->threads )
			th.join();

		for( auto& cnt: counters )
		{
			cnt.stats();
		}

		this->ready = false;
		this->pause = false;

		return counters;
	}

	template<typename EventTyp>
	std::vector<CntTyp> counters_with_priority_schedule(
	    std::vector<Schedule<EventTyp, CntTyp>>& svec, int warmup = 0 )
	{
		std::vector<CntTyp> counters{ svec.size() };
		for( int i = 0; i < svec.size(); ++i )
		{
			counters[i].label = svec[i].label;
			// Could use condition variable instead of futures here as well
			this->threads.push_back(
			    std::thread( [this, &counters, &svec, i, warmup] {
				    this->counter_thread_fn<EventTyp>(
				        counters[i], svec[i].events, 0 /* thread id */,
				        svec[i].core_id, svec[i].benchmark, warmup );
			    } ) );
			pin_to_core( 0, svec[i].core_id );

			// Start scheduled thread ready
			{
				std::lock_guard<std::mutex> lck( this->mtx );
				this->pause = false;
			}
			this->cv.notify_one();

			{
				std::unique_lock<std::mutex> lck( this->mtx );
				auto wakeup = [this]() { return this->ready == true; };
				this->cv.wait( lck, wakeup );
			}
			for( auto& th: this->threads )
				th.join();

			this->threads.clear();
		}

		this->ready = false;
		this->pause = false;
		return counters;
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
