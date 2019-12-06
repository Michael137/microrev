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
#include "counters.h"
#include "shuffle.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

using namespace pcnt;

BENCHMARK_SETUP();

int main( int argc, char* argv[] )
{
#ifdef WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC
	benchmark_setup();

	std::vector<std::vector<std::string>> cnt_vec_list;
	cnt_vec_list.push_back(
	    std::vector<std::string>{ "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES",
	                              "perf::L1-DCACHE-LOADS" } );
	cnt_vec_list.push_back( std::vector<std::string>{
	    "L2_LINES_IN:ALL", "L2_LINES_IN:E", "L2_LINES_IN:I" } );
	cnt_vec_list.push_back( std::vector<std::string>{ "L2_LINES_IN:ALL:cpu=1",
	                                                  "L2_LINES_IN:E:cpu=1",
	                                                  "L2_LINES_IN:I:cpu=1" } );
	cnt_vec_list.push_back( std::vector<std::string>{ "L2_LINES_IN:ALL:cpu=7",
	                                                  "L2_LINES_IN:E:cpu=7",
	                                                  "L2_LINES_IN:I:cpu=7" } );
	cnt_vec_list.push_back( std::vector<std::string>{ "L2_LINES_IN:ALL:cpu=9",
	                                                  "L2_LINES_IN:E:cpu=9",
	                                                  "L2_LINES_IN:I:cpu=9" } );
	cnt_vec_list.push_back( std::vector<std::string>{ "L2_LINES_IN:S" } );
	cnt_vec_list.push_back( std::vector<std::string>{
	    "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITMESF:SNP_ANY" } );
	cnt_vec_list.push_back( std::vector<std::string>{
	    "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITM:SNP_ANY" } );
	cnt_vec_list.push_back( std::vector<std::string>{
	    "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITS:SNP_ANY" } );
	cnt_vec_list.push_back( std::vector<std::string>{
	    "PAPI_L2_DCA", "PAPI_L2_DCM", "PAPI_L3_DCA" } );
	cnt_vec_list.push_back( std::vector<std::string>{ "" } );

	for( auto c: cnt_vec_list )
	{
		for( int j = 0; j < 3; j++ )
		{
			for( int i = 0; i < 100; i++ )
			{
				run_test( LOAD_FROM_MODIFIED,
				          static_cast<core_placement_t>( j ), c );
				run_test( LOAD_FROM_SHARED, static_cast<core_placement_t>( j ),
				          c );
				run_test( LOAD_FROM_INVALID, static_cast<core_placement_t>( j ),
				          c );
				run_test( FLUSH, static_cast<core_placement_t>( j ), c );
			}
		}
	}

	free( (void*)shared_data );

#endif // !WITH_PAPI_LL

	return 0;
}
