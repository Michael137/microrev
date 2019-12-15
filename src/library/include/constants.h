#ifndef CONSTANTS_H_IN
#define CONSTANTS_H_IN

namespace pcnt
{
// Number of simultaneous PMCs that
// platform permits
constexpr int max_pmc_num = 4;

constexpr unsigned int default_phys_core_count = 4;

constexpr uint64_t _8B    = 8;
constexpr uint64_t _16B   = 16;
constexpr uint64_t _32B   = 32;
constexpr uint64_t _64B   = 64;
constexpr uint64_t _128B  = 128;
constexpr uint64_t _256B  = 256;
constexpr uint64_t _512B  = 512;
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
constexpr uint64_t _8MB   = 8388608;
constexpr uint64_t _16MB  = 16777216;

#ifdef __llvm__
#	define OPT0 __attribute__( ( optnone ) )
#else
#	define OPT0 __attribute__( ( optimize( "O0" ) ) )
#endif

} // namespace pcnt

#endif // CONSTANTS_H_IN
