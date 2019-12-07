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
    for( int i = 0; i < 10; i++ )
    {
        for ( uint64_t n = 1; n * cache_size < _4MB; n*=2) {
            setup(cache_size * n, cache_size);

            std::vector<Sched> vec{
                { n % 10, std::function<decltype( reader )>{ reader }, {"PAPI_L1_DCM", "L1D:REPLACEMENT", "PAPI_TOT_CYC"}, std::to_string(n)}};

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
