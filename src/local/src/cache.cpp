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
doloops( uint64_t wset_sz, uint64_t stride = 4096, uint64_t repeat = 100000 )
{
	char c;
	char arr[wset_sz];
	for( uint64_t i = 0; i < wset_sz; i += stride )
		arr[i] = ( i % 9 ) + 'A' - 1;

	for( uint64_t i = 0; i < repeat; ++i )
		for( uint64_t j = 0; j < wset_sz; j += stride )
			c = arr[j];
}

void test_wset2() { doloops( _2KB ); }
void test_wset4() { doloops( _4KB ); }
void test_wset8() { doloops( _8KB ); }
void test_wset16() { doloops( _16KB ); }
void test_wset32() { doloops( _32KB ); }
void test_wset64() { doloops( _64KB ); }
void test_wset128() { doloops( _128KB ); }
void test_wset256() { doloops( _256KB ); }
void test_wset512() { doloops( _512KB ); }
void test_wset1024() { doloops( _1MB ); }
void test_wset2048() { doloops( _2MB ); }

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
	using Sched = Schedule<std::vector<std::string>>;

	Sched core_1
	    = Sched{ 1,
	             std::function<decltype( test_wset32 )>{ test_wset32 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	//"CYCLE_ACTIVITY:CYCLES_L1D_PENDING" } };
	Sched core_2
	    = Sched{ 2,
	             std::function<decltype( test_wset32 )>{ test_wset256 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	//"CYCLE_ACTIVITY:CYCLES_L1D_PENDING" } };
	Sched core_3
	    = Sched{ 3,
	             std::function<decltype( test_wset32 )>{ test_wset512 },
	             { "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" } };
	//"CYCLE_ACTIVITY:CYCLES_L1D_PENDING" } };

	std::vector<Sched> vec{ core_1, core_2, core_3 };
	cbench.counters_with_schedule<std::vector<std::string>>( vec );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;
}
