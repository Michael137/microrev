#include <chrono>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include <string>

#include "constants.h"
#include "counter.h"
#include "thread_utils.h"
#include "shuffle.h"
#include "benchmark_helpers.h"

using namespace pcnt;


BENCHMARK_INIT();

int main( int argc, char** argv )
{
	/* Should allow us to see cache sizes */
    parse_cfg();
	CounterBenchmark<PAPILLCounter> cbench;
    for( int i = 0; i < 1; i++ )
    {
        for ( uint64_t n = _4MB / cache_size; n >= 1; n/=2) {
            std::cout<< n<< std::endl;
            shared_data_size = cache_size * n;
            setup(shared_data_size, cache_size);

            //std::vector<Sched> vec{
            //    { 1, std::function<decltype( reader )>{ reader }, {}, std::to_string(n)}};
            
            
            std::vector<Sched> vec{
                { 1, std::function<decltype( reader )>{ reader }, {"perf::PERF_COUNT_HW_CACHE_L1D:ACCESS", "perf::PERF_COUNT_HW_CACHE_L1D:PREFETCH"}, std::to_string(n)},
                { 1, std::function<decltype( reader )>{ reader }, {"PAPI_L1_DCM", "L1D:REPLACEMENT", "PAPI_TOT_CYC"}, std::to_string(n)},
                { 1, std::function<decltype( reader )>{ reader }, {"PAPI_L2_DCM", "PAPI_L2_DCH"}, std::to_string(n)}};

            auto counters
                = cbench.counters_with_priority_schedule<std::vector<std::string>>(
                    vec, 5 );
            free((void *)shared_data);
            for( auto& c: counters )
                c.stats();
        }
    }
	return 0;
}
