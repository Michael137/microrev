#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "counter.h"
#include "counters.h"
#include "thread_utils.h"

#if defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

struct RandomInt
{
	std::random_device rd;
	std::mt19937 mt;
	std::uniform_int_distribution<int> dist;

	RandomInt()
	    : rd()
	    , mt( rd() )
	    , dist( 1, 1000000 )
	{
	}

	int gen() { return dist( this->mt ); }
};

static RandomInt rand_gen;
static const int rand_g = rand_gen.gen();

void do_flops( pcnt::PAPILLCounter& pc )
{
	uint64_t start, end;
	pc.start();
	start   = pcnt::rdtsc();
	float x = 0.0;
	for( int i = 0; i < rand_g; ++i )
		x *= 0.2;
	end = pcnt::rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

void do_ints( pcnt::PAPILLCounter& pc )
{
	uint64_t start, end;
	pc.start();
	start = pcnt::rdtsc();
	int x = 0;
	for( int i = 0; i < rand_g; ++i )
		x *= 0;
	end = pcnt::rdtsc();
	pc.read();
	pc.cycles_measured = end - start;
}

int main( int argc, char* argv[] )
{
	std::cout << ">>>> TEST: different benchmarks on different cores "
	             "(validates counters are read correctly on separate cores)"
	          << std::endl;
#ifdef WITH_PMC

	pcnt::CounterBenchmark<pcnt::PMCCounter> cbench;

	using Sched          = pcnt::Schedule<std::vector<std::string>>;
	auto FreeBSDCounters = pcnt::CounterMap["FreeBSD"];

	Sched core_1 = Sched{ 1, std::function<decltype( do_flops )>{ do_flops },
	                      FreeBSDCounters["icache"] };
	Sched core_2 = Sched{ 2, std::function<decltype( do_ints )>{ do_ints },
	                      FreeBSDCounters["dcache"] };
	Sched core_3 = Sched{
	    3,
	    std::function<decltype( do_ints )>{ do_ints },
	    { "arith.fpu_div_active", "arith.fpu_div", "fp_comp_ops_exe.x87" } };

	std::vector<Sched> vec{ core_1, core_2, core_3 };

	cbench.counters_with_schedule<std::vector<std::string>>( vec );

#elif defined( WITH_PAPI_HL ) // !WITH_PMC

#	error "TODO: implement this test using PAPI's HL interface"

#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL

	pcnt::CounterBenchmark<pcnt::PAPILLCounter> cbench;

	using Sched = pcnt::Schedule<std::vector<std::string>, pcnt::PAPILLCounter>;

	Sched core_1 = Sched{
	    1,
	    std::function<decltype( do_flops )>{ do_flops },
	    { "PAPI_FP_INS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};
	Sched core_2 = Sched{
	    2,
	    std::function<decltype( do_ints )>{ do_ints },
	    { "PAPI_FP_INS", "PAPI_TOT_INS", "PAPI_TOT_CYC" },
	};
	Sched core_3 = Sched{
	    3,
	    std::function<decltype( do_ints )>{ do_ints },
	    { "PAPI_L3_TCW", "PAPI_L2_DCH", "PAPI_TOT_CYC" },
	};

	std::vector<Sched> vec{ core_1, core_2, core_3 };

	auto counters = cbench.counters_with_schedule<std::vector<std::string>>( vec );

	for(auto& c : counters)
		c.print_stats();

#endif // !WITH_PAPI_LL
	std::cout << ">>>> TEST COMPLETED <<<<" << std::endl;

	return 0;
}
