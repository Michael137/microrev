#include <immintrin.h>
#include <x86intrin.h>
#include <cstdint>

#include "time_utils.h"

namespace pcnt
{
// Returns number of cycles
uint64_t rdtsc()
{
	_mm_lfence();
	uint64_t ret = __rdtsc();
	_mm_lfence();
	return ret;
#if 0
	uint32_t hi, lo;

	// lfence to serialize timer
	// TODO: need to specify clobber list?
	asm volatile( "lfence" ::: "eax", "ebx", "ecx", "edx" );
	asm volatile( "rdtsc" : "=a"( lo ), "=d"( hi ) );

	return ( static_cast<uint64_t>( hi ) << 32 ) | lo;
#endif
}

} // namespace pcnt
