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

#define KB1 1024
#define KB2 2048
#define KB4 4096
#define KB8 8192
#define KB16 16384
#define KB32 32768
#define KB64 65536
#define KB128 131072
#define KB256 262144
#define KB512 524288
#define MB1 1048576
#define MB2 2097152
#define MB4 4194304

} // namespace pcnt

#endif // CONSTANTS_H_IN
