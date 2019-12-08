#ifndef THREAD_UTILS_H_IN
#define THREAD_UTILS_H_IN

#include <pthread.h>

#ifdef __FreeBSD__
#	include <pthread_np.h>
#endif

#include <immintrin.h>
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

template<typename CntTyp> class Stoppable
{
	std::promise<void> exitSignal;
	std::future<void> futureObj;

   public:
	Stoppable()
	    : futureObj( exitSignal.get_future() )
	{
	}
	Stoppable( Stoppable&& obj )
	    : exitSignal( std::move( obj.exitSignal ) )
	    , futureObj( std::move( obj.futureObj ) )
	{
	}

	Stoppable& operator=( Stoppable&& obj )
	{
		exitSignal = std::move( obj.exitSignal );
		futureObj  = std::move( obj.futureObj );
		return *this;
	}

	virtual void run( CntTyp& ) = 0;

	void operator()() { run(); }

	bool stopRequested()
	{
		if( futureObj.wait_for( std::chrono::milliseconds( 0 ) )
		    == std::future_status::timeout )
			return false;
		return true;
	}
	void stop() { exitSignal.set_value(); }
};

template<typename CntTyp> class MeasurementBench : public Stoppable<CntTyp>
{
   public:
	void run( CntTyp& pc )
	{
		pc.start();
		while( this->stopRequested() == false )
		{
			_mm_pause();
		}
		pc.read();
	}
};

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

   public:
	std::mutex mtx;
	std::condition_variable cv;
	unsigned int benchmark_cores;
	std::vector<std::thread> threads;
	std::atomic<bool> pause;

	CounterBenchmark()
	    : mtx()
	    , cv()
	    , benchmark_cores( default_phys_core_count )
	    , threads()
	    , pause( false )
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

	void pause_thread()
	{
		if(!this->pause)
		{
			std::lock_guard<std::mutex> lck(this->mtx);
			this->pause = true;
		}
	}

	void unpause_thread()
	{
		if(this->pause)
		{
			{
				std::lock_guard<std::mutex> lck(this->mtx);
				this->pause = false;
			}	
			this->cv.notify_all();
		}
	}

	void wait_to_start()
	{
			std::unique_lock<std::mutex> lck(this->mtx);
			auto not_paused = [this](){return this->pause == false;};
			this->cv.wait(lck, not_paused);
	}

	template<typename EventTyp>
	void counter_thread_fn( CntTyp& counter, EventTyp& events, int th_id,
	                        int core_id, BenchTyp benchmark, int warmup = 5 )
	{
		uint64_t start, end;
		counter.core_id = core_id;

		this->wait_to_start();

		counter.add( events );

		for( int i = 0; i < warmup; ++i )
			benchmark( counter );

		counter.reset();

		benchmark( counter );

		counter.end();
		pause_thread();
	}

	template<typename EventTyp>
	std::vector<CntTyp> counters_with_schedule( std::vector<Schedule<EventTyp, CntTyp>>& svec )
	{
		std::vector<CntTyp> counters{ svec.size() };
		for( int i = 0; i < svec.size(); ++i )
		{
			counters[i].label = svec[i].label;
			this->threads.push_back( std::thread( [this, &counters, &svec, i] {
				this->counter_thread_fn<EventTyp>( counters[i], svec[i].events,
				                                   i, svec[i].core_id,
				                                   svec[i].benchmark );
			} ) );
			pin_to_core( i, svec[i].core_id );
		}

		unpause_thread();

		for( auto& th: this->threads )
			th.join();

		for( auto& cnt: counters )
		{
			cnt.stats();
		}
		return counters;
	}

	template<typename EventTyp>
	std::vector<CntTyp> counters_with_priority_schedule(
	    std::vector<Schedule<EventTyp, CntTyp>>& svec, int warmup = 0 )
	{
		std::vector<CntTyp> counters{ svec.size() };
		std::vector<CntTyp> measurement_counters;
		std::vector<MeasurementBench<CntTyp>> measurement_tasks;
		for( int i = 0; i < svec.size(); ++i )
		{
			counters[i].label = svec[i].label;
			// Could use condition variable instead of futures here as well
			std::packaged_task<void()> task(
			    [this, &counters, &svec, i, warmup] {
				    this->counter_thread_fn<EventTyp>(
				        counters[i], svec[i].events, 0 /* thread id */,
				        svec[i].core_id, svec[i].benchmark, warmup );
			    } );
			auto fut = task.get_future();

			this->threads.push_back( std::thread( std::move( task ) ) );
			pin_to_core( 0, svec[i].core_id );
			unpause_thread();

//			auto msched_size = svec[i].measurement_scheds.size();
//			measurement_counters.resize( measurement_counters.size()
//			                             + msched_size );
//			for( int j = 0; j < msched_size; ++j )
//			{
//				measurement_tasks.emplace_back();
//				this->threads.push_back(
//				    std::thread( [this, &measurement_counters, &svec, j, i,
//				                  &measurement_tasks] {
//					    this->counter_thread_fn<EventTyp>(
//					        measurement_counters[j],
//					        svec[i].measurement_scheds[j].events,
//					        j + 1 /* thread id */,
//					        svec[i].measurement_scheds[j].core_id,
//					        [&]( CntTyp& pc ) {
//						        measurement_tasks[j].run( pc );
//					        },
//					        0 /* warmup */ );
//				    } ) );
//				pin_to_core( j + 1, svec[i].measurement_scheds[j].core_id );
			//}

			auto status = fut.wait_for( std::chrono::seconds( 30 ) );

			if( status == std::future_status::ready )
			{
			//	for( auto& task: measurement_tasks )
			//		task.stop();
				for( auto& th: this->threads )
					th.join();

				this->threads.clear();

			//	counters.insert( counters.end(), measurement_counters.begin(),
				                 //measurement_counters.end() );
			//	measurement_counters.clear();
			//	measurement_tasks.clear();

				continue;
			}
			else
			{
				std::cout << "Serial benchmark timelimit reached." << std::endl;
				exit( EXIT_FAILURE );
			}
		}

		return counters;
	}
};

} // namespace pcnt

#endif // THREAD_UTILS_H_IN
