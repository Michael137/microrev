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

void func( void ) {
	std::cout << "Test Func" << std::endl;
}

int main( int argc, char* argv[] )
{
	std::cout << ">>>> TEST: force caches into M/E/S/I and measure performance"
	          << std::endl;
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	using Sched = pcnt::Schedule<std::vector<std::string>>;

	Sched core_1 = Sched{
	    1 /* core id */,
	    std::function<decltype( func )>{ func },
	    { "FP_ARITH:SCALAR_DOUBLE", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	    false /* collect */
	};
	Sched core_2 = Sched{
	    2 /* core id */,
	    std::function<decltype( func )>{ func },
	    { "FP_ARITH:SCALAR_DOUBLE", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	    true /* collect */
	};
	Sched core_3 = Sched{
	    3 /* core id */,
	    std::function<decltype( func )>{ func },
	    { "FP_ARITH:SCALAR_DOUBLE", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	    false /* collect */
	};

	std::vector<Sched> vec{ core_1, core_2, core_3 };

	cbench.counters_with_priority_schedule<std::vector<std::string>>( vec );

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
