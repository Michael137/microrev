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

} // namespace pcnt
