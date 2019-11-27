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

constexpr uint64_t _1KB   = 1024;
constexpr uint64_t _2KB   = 2048;
constexpr uint64_t _4KB   = 4096;
constexpr uint64_t _8KB   = 8192;
constexpr uint64_t _16KB  = 16384;
constexpr uint64_t _32KB  = 32768;
constexpr uint64_t _64KB  = 65536;
constexpr uint64_t _128KB = 131072;
constexpr uint64_t _256KB = 262144;
constexpr uint64_t _512KB = 524288;
constexpr uint64_t _1MB   = 1048576;
constexpr uint64_t _2MB   = 2097152;
constexpr uint64_t _4MB   = 4194304;

} // namespace pcnt

#endif // CONSTANTS_H_IN
