#ifndef BENCHMARK_HELPERS_H_IN
#define BENCHMARK_HELPERS_H_IN

#include <inttypes.h>
#include <string>
#include <vector>

#include "constants.h"
#include "counter.h"
#include "thread_utils.h"

extern volatile char* shared_data;
extern volatile char** shared_iter;
extern volatile uint64_t shared_data_size;
extern volatile uint64_t cache_line_size;
extern volatile uint64_t cache_size;
extern volatile uint64_t stride;

extern int core_src;
extern int core_socket0;
extern int core_socket1;
extern int core_global0;
extern int core_global1;

#define BENCHMARK_INIT()                                                       \
	volatile char* shared_data         = nullptr;                              \
	volatile char** shared_iter        = nullptr;                              \
	volatile uint64_t shared_data_size = 0;                                    \
	volatile uint64_t cache_line_size  = 0;                                    \
	volatile uint64_t cache_size       = 0;                                    \
	volatile uint64_t stride       = 0;                                    \
                                                                               \
	int core_src     = 0;                                                      \
	int core_socket0 = 0;                                                      \
	int core_socket1 = 0;                                                      \
	int core_global0 = 0;                                                      \
	int core_global1 = 0;

// Pre-defined benchmark function names
void OPT0 reader( pcnt::PAPILLCounter& pc );
void OPT0 writer( pcnt::PAPILLCounter& pc );
void OPT0 flusher( pcnt::PAPILLCounter& pc );

enum cache_state_t : unsigned int
{
	M_STATE,
	E_STATE,
	S_STATE,
	I_STATE,
	O_STATE,
	F_STATE
};

enum mesi_type_t : unsigned int
{
	STORE_ON_MODIFIED,
	STORE_ON_EXCLUSIVE,
	STORE_ON_SHARED,
	STORE_ON_INVALID,
	LOAD_FROM_MODIFIED,
	LOAD_FROM_EXCLUSIVE,
	LOAD_FROM_SHARED,
	LOAD_FROM_INVALID,
	FLUSH,
};

enum core_placement_t : unsigned int
{
	LOCAL,  // Same core
	SOCKET, // Same Socket
	GLOBAL  // Across Sockets
};

using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

void benchmark_setup();
void set_state( std::vector<Sched>& vec, uint64_t cc_state, int core_a,
                int core_b, std::vector<std::string> cnt_vec );
void run_test( mesi_type_t t, core_placement_t c,
               std::vector<std::string> cnt_vec );

#endif // BENCHMARK_HELPERS_H_IN
