#ifndef PMC_COUNTERS_H_IN
#define PMC_COUNTERS_H_IN

#include <string>
#include <unordered_map>
#include <vector>

namespace pmc_utils
{
using CounterSet = std::vector<std::string>;

// Maps OS to ready-to-use performance-counter sets
static std::unordered_map<std::string,
                                std::unordered_map<std::string, CounterSet>>
    CounterMap
    = {{"FreeBSD",
        {{"icache", {"icache.hit", "icache.misses", "icache.ifetch_stall"}},
         {"dcache",
          {"mem_load_uops_retired.l1_hit", "mem_load_uops_retired.l1_miss"}}}}};

} // namespace pmc_utils

#endif // PMC_COUNTERS_H_IN
