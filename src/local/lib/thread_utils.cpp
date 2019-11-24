#include <pthread.h>
#include <cstdint>

#ifdef __FreeBSD__
#	include <pthread_np.h>
#endif

#include "thread_utils.h"

#ifdef __FreeBSD__
#	define CPUSET_T cpuset_t
#else
#	define CPUSET_T cpu_set_t
#endif

namespace pcnt
{
void pin_self_to_core( int core_id )
{
	// Master thread on core 0
	CPUSET_T cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( core_id, &cpuset );

	pthread_setaffinity_np( pthread_self(), sizeof( CPUSET_T ), &cpuset );
}

// Returns number of cycles
uint64_t rdtsc()
{
	uint32_t hi, lo;

	// lfence to serialize timer
	// TODO: need to specify clobber list?
	asm volatile( "lfence" ::: "eax", "ebx", "ecx", "edx" );
	asm volatile( "rdtsc" : "=a"( lo ), "=d"( hi ) );

	return ( static_cast<uint64_t>( hi ) << 32 ) | lo;
}

} // namespace pcnt
