#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "constants.h"
#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

void __attribute__( ( optimize( "0" ) ) )
doloops( PAPILLCounter& pc, uint64_t wset_sz, uint64_t stride = 32,
         uint64_t repeat = 100 )
{
	uint64_t start, end;
	char c;
	char arr[wset_sz];
	for( uint64_t i = 0; i < wset_sz; i += stride )
		arr[i] = ( i % 9 ) + 'A' - 1;

	pc.start();
	start = rdtsc();
	for( uint64_t i = 0; i < repeat; ++i )
		for( uint64_t j = 0; j < wset_sz; j += stride )
			c = arr[j];
	end = rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

void test_wset2( PAPILLCounter& pc ) { doloops( pc, _2KB ); }
void test_wset4( PAPILLCounter& pc ) { doloops( pc, _4KB ); }
void test_wset8( PAPILLCounter& pc ) { doloops( pc, _8KB ); }
void test_wset16( PAPILLCounter& pc ) { doloops( pc, _16KB ); }
void test_wset32( PAPILLCounter& pc ) { doloops( pc, _32KB ); }
void test_wset64( PAPILLCounter& pc ) { doloops( pc, _64KB ); }
void test_wset128( PAPILLCounter& pc ) { doloops( pc, _128KB ); }
void test_wset256( PAPILLCounter& pc ) { doloops( pc, _256KB ); }
void test_wset512( PAPILLCounter& pc ) { doloops( pc, _512KB ); }
void test_wset1024( PAPILLCounter& pc ) { doloops( pc, _1MB ); }
void test_wset2048( PAPILLCounter& pc ) { doloops( pc, _2MB ); }

int main( int argc, char* argv[] )
{
	std::cout << ">>>> TEST: increase working set until cache sizes are "
	             "exceeded from L1 to LLC; access data in non-unit strides "
	             "(hint: turn prefetchers on/off)"
	          << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PMC"

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>, PAPILLCounter>;

	Sched core_1
	    = Sched{ 1,
	             std::function<decltype( test_wset32 )>{ test_wset2048 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	Sched core_2
	    = Sched{ 2,
	             std::function<decltype( test_wset32 )>{ test_wset1024 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	Sched core_3
	    = Sched{ 3,
	             std::function<decltype( test_wset32 )>{ test_wset512 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };

	std::vector<Sched> vec{ core_1, core_2, core_3 };
	cbench.counters_with_schedule<std::vector<std::string>>( vec );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;
}
