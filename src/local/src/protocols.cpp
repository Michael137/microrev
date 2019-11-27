#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <vector>

#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

volatile char* shared_data    = nullptr;
volatile uint64_t shared_data_size = _16KB;

void writer()
{
	for( uint64_t i = 0; i < shared_data_size; ++i )
		shared_data[i] = ( i % 26 ) + '0';
}

void reader()
{
	char tmp;
	for( uint64_t i = 1; i < shared_data_size; ++i )
		tmp = shared_data[i] - shared_data[i - 1];
}

int main( int argc, char* argv[] )
{
	std::cout
	    << ">>>> TEST: force caches into M/E/S/I/F and measure performance"
	    << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	/* thread 1: write to shared_data
	 * thread 2: read from shared_data (now cache in core 1 and 2 should be in S
state)
	 * thread 3 to N: read from shared_data (in MESIF protocol the traffic
produced should be the same as if only a single core is asking for the data) */

	shared_data = (char*)malloc( sizeof( char ) * shared_data_size );

	CounterBenchmark<PAPILLCounter> cbench;
	using Sched = Schedule<std::vector<std::string>>;

	Sched core_1 = Sched{
	    1 /* core id */
	    ,
	    std::function<decltype( writer )>{ writer },
	    { "OFFCORE_RESPONSE_1:L3_HITMESF", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	    true /* collect */
	};
	Sched core_2 = Sched{
	    2 /* core id */,
	    std::function<decltype( writer )>{ reader },
	    { "OFFCORE_RESPONSE_1:L3_HITMESF", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	    true /* collect */
	};
	Sched core_3 = Sched{
	    3 /* core id */,
	    std::function<decltype( writer )>{ reader },
	    { "OFFCORE_RESPONSE_1:L3_HITMESF", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	    true /* collect */
	};

	std::vector<Sched> vec{ core_1, core_2, core_3 };

	cbench.counters_with_priority_schedule<std::vector<std::string>>( vec );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
