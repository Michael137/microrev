#ifndef COUNTERS_H_IN
#define COUNTERS_H_IN

#include <string>
#include <unordered_map>
#include <vector>

namespace pcnt
{
using CounterSet = std::vector<std::string>;

// Maps OS to ready-to-use performance-counter sets
// pmc
static std::unordered_map<std::string,
                          std::unordered_map<std::string, CounterSet>>
    CounterMap
    = {{"FreeBSD",
        {{"icache", {"icache.hit", "icache.misses", "icache.ifetch_stall"}},
         {"dcache",
          {"mem_load_uops_retired.l1_hit", "mem_load_uops_retired.l1_miss"}}}}};

} // namespace pcnt

#endif // COUNTERS_H_IN
