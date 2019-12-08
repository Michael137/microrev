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

BENCHMARK_INIT();

int main( int argc, char* argv[] )
{
#ifdef WITH_PMC

#error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PMC

	parse_cfg();

    std::vector<std::vector<std::string> > cnt_vec_list;
    cnt_vec_list.push_back(std::vector<std::string> { "PAPI_TOT_INS", "perf::L1-DCACHE-LOAD-MISSES", "perf::L1-DCACHE-LOADS" });
    //cnt_vec_list.push_back(std::vector<std::string> { "L2_RQSTS:CODE_RD_HIT", "L2_RQSTS:CODE_RD_MISS", "L2_RQSTS:ALL_PF"});
    /*
    cnt_vec_list.push_back(std::vector<std::string> { "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITMESF:SNP_ANY"}); 
    cnt_vec_list.push_back(std::vector<std::string> { "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITM:SNP_ANY"}); 
    cnt_vec_list.push_back(std::vector<std::string> { "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITS:SNP_ANY"}); 
    cnt_vec_list.push_back(std::vector<std::string> { "PAPI_L2_DCA", "PAPI_L2_DCM", "PAPI_L3_DCA"});
    cnt_vec_list.push_back(std::vector<std::string> { "perf::PERF_COUNT_HW_CACHE_LL:ACCESS", "perf::PERF_COUNT_HW_CACHE_LL:MISS"});
    */
    //cnt_vec_list.push_back(std::vector<std::string> { "ix86arch:LLC_MISSES"}); 
    //cnt_vec_list.push_back(std::vector<std::string> { "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITE:SNP_ANY", "OFFCORE_RESPONSE_0:ANY_DATA:L3_MISS:SNP_ANY" });
    //std::vector<uint64_t> size_vec{_1KB, _2KB, _4KB, _8KB, _16KB, _32KB, _64KB, _128KB, _256KB, _512KB, _1MB, _2MB};
    std::vector<uint64_t> size_vec{_4KB, _8KB, _16KB, _32KB, _64KB, _128KB, _256KB, _512KB, _1MB, _2MB};

    for(auto s : size_vec)
	{
	        shared_data_size = s;
	    	pr_co_setup( shared_data_size );
		for(int i = 0; i < 3; ++i) {
			for(int j = 0; j < 100; ++j) {
			    run_test( FLUSH, (core_placement_t)i , { "perf::L1-DCACHE-LOAD-MISSES"});
			    run_test( PRODUCER_CONSUMER, (core_placement_t)i , { "perf::L1-DCACHE-LOAD-MISSES"});
			}
		}
	}

    // free((void*) shared_data);
    // free((void*) wrflag);

#endif // !WITH_PAPI_LL

	return 0;
}
