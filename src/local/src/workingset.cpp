#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

template<int sz> struct wset
{
	char data[sz];
};

wset<1024> ws1;
wset<2048> ws2;
wset<4096> ws4;
wset<8192> ws8;
wset<16384> ws16;
wset<32768> ws32;
wset<65536> ws64;

void doloops( int stride )
{
	char arr[1000000];
	for( int i = 0; i < 1000000; i += stride )
		for( int j = i; j < stride; ++j )
			arr[j] = ( i % 26 ) + '0';
}

void test_wset2() { doloops( 2048 ); }
void test_wset4() { doloops( 4096 ); }
void test_wset8() { doloops( 8192 ); }
void test_wset16() { doloops( 16384 ); }
void test_wset32() { doloops( 32768 ); }
void test_wset64() { doloops( 65536 ); }

int main( int argc, char* argv[] )
{
	std::cout << ">>>> TEST: increase working set until cache sizes are "
	             "exceeded from L1 to LLC"
	          << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;
	using Sched = pcnt::Schedule<std::vector<std::string>>;

	Sched core_1 = Sched{
	    1,
	    std::function<decltype( test_wset2 )>{ test_wset2 },
	    { "perf::IDLE-CYCLES-BACKEND", "PAPI_TOT_CYC", "PAGE_WALKS:LLC_MISS" },
	};
	Sched core_2 = Sched{
	    2,
	    std::function<decltype( test_wset4 )>{ test_wset4 },
	    { "perf::IDLE-CYCLES-BACKEND", "PAPI_TOT_CYC", "PAGE_WALKS:LLC_MISS" },
	};
	Sched core_3 = Sched{
	    3,
	    std::function<decltype( test_wset8 )>{ test_wset8 },
	    { "perf::IDLE-CYCLES-BACKEND", "PAPI_TOT_CYC", "PAGE_WALKS:LLC_MISS" },
	};
	std::vector<Sched> vec{ core_1, core_2, core_3 };
	cbench.counters_with_schedule<std::vector<std::string>>( vec );

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench2;
	Sched core_1a = Sched{
	    1,
	    std::function<decltype( test_wset16 )>{ test_wset16 },
	    { "perf::IDLE-CYCLES-BACKEND", "PAPI_TOT_CYC", "PAGE_WALKS:LLC_MISS" },
	};
	Sched core_2a = Sched{
	    2,
	    std::function<decltype( test_wset32 )>{ test_wset32 },
	    { "perf::IDLE-CYCLES-BACKEND", "PAPI_TOT_CYC", "PAGE_WALKS:LLC_MISS" },
	};
	Sched core_3a = Sched{
	    3,
	    std::function<decltype( test_wset64 )>{ test_wset64 },
	    { "perf::IDLE-CYCLES-BACKEND", "PAPI_TOT_CYC", "PAGE_WALKS:LLC_MISS" },
	};
	std::vector<Sched> vec2 = { core_1a, core_2a, core_3a };
	cbench2.counters_with_schedule<std::vector<std::string>>( vec2 );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;
}
