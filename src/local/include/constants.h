#ifndef CONSTANTS_H_IN
#define CONSTANTS_H_IN

namespace pmc_utils
{

#ifdef __FreeBSD__
// Number of simultaneous PMCs that
// platform permits
constexpr int max_pmc_num = 4;
#else
constexpr int max_pmc_num = 8;
#endif

} // namespace pmc_utils

#endif // CONSTANTS_H_IN
