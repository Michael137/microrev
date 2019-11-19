#ifndef CONSTANTS_H_IN
#define CONSTANTS_H_IN

namespace pcnt
{

#ifdef __FreeBSD__
// Number of simultaneous PMCs that
// platform permits
constexpr int max_pmc_num = 4;
#else
constexpr int max_pmc_num = 8;
#endif

constexpr unsigned int default_phys_core_count = 4;

} // namespace pcnt

#endif // CONSTANTS_H_IN
