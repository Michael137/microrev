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

void doloops( int stride )
{
	char arr[MB4];
	for( int i = 0; i < MB4; i += stride )
		for( int j = 0; j < stride; ++j )
			arr[j + i] = ( j % 9 ) + 'A' - 1;
}

void test_wset2() { doloops( KB2 ); }
void test_wset4() { doloops( KB4 ); }
void test_wset8() { doloops( KB8 ); }
void test_wset16() { doloops( KB16 ); }
void test_wset32() { doloops( KB32 ); }
void test_wset64() { doloops( KB64 ); }
void test_wset128() { doloops( KB128 ); }
void test_wset256() { doloops( KB256 ); }
void test_wset512() { doloops( KB512 ); }
void test_wset1024() { doloops( MB1 ); }

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

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;
	using Sched = pcnt::Schedule<std::vector<std::string>>;

	Sched core_1 = Sched{ 1,
	                      std::function<decltype(test_wset32)>{ test_wset32 },
	                      { "PAPI_TOT_CYC", "PAPI_L2_DCA", "PAPI_L3_DCA" } };
	Sched core_2 = Sched{ 2,
	                      std::function<decltype(test_wset32)>{ test_wset64 },
	                      { "PAPI_TOT_CYC", "PAPI_L2_DCA", "PAPI_L3_DCA" } };
	Sched core_3 = Sched{ 3,
	                      std::function<decltype(test_wset32)>{ test_wset128 },
	                      { "PAPI_TOT_CYC", "PAPI_L2_DCA", "PAPI_L3_DCA" } };
	std::vector<Sched> vec{ core_1, core_2, core_3 };
	cbench.counters_with_schedule<std::vector<std::string>>( vec );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;
}
