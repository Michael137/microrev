#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "benchmark_helpers.h"
#include "counter.h"
#include "shuffle.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

BENCHMARK_INIT();

int main( int argc, char* argv[] )
{
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC

	parse_cfg();

	std::vector<uint64_t> size_vec{ _4KB,   _8KB,   _16KB,  _32KB, _64KB,
	                                _128KB, _256KB, _512KB, _1MB,  _2MB };

	for( auto s: size_vec )
	{
		shared_data_size = s;
		pr_co_setup( shared_data_size );
		for( int i = 0; i < 3; ++i )
		{
			for( int j = 0; j < 100; ++j )
			{
				run_test( FLUSH, (core_placement_t)i,
				          { "perf::L1-DCACHE-LOAD-MISSES" } );
				run_test( PRODUCER_CONSUMER, (core_placement_t)i,
				          { "perf::L1-DCACHE-LOAD-MISSES" } );
			}
		}
	}

#endif // !WITH_PAPI_LL

	return 0;
}
