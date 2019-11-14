#ifndef COUNTER_H_IN
#define COUNTER_H_IN

#include <string>
#include <vector>

#if defined(__FreeBSD__) && defined(WITH_PMC)
	#include <err.h>
	#include <pmc.h>
	#include <sysexits.h>
#endif

#include "counters.h"

namespace pcnt
{
// List of counters: `pmccontrol -L`
template<typename CntResult>
class Counter
{
   protected:
	CounterSet cset;
	std::vector<CntResult> measured;

   public:
	Counter();
	explicit Counter( CounterSet const& cset );
	void add( std::string counter_name ) = 0;
	void add( CounterSet const& cset ) = 0;
	void read() = 0;
	void stats() = 0;
	void start();
};

#ifdef WITH_PMC
class PMCCounter : public Counter<pmc_value_t>
{
   private:
	pmc_id_t pmcid;
	enum pmc_mode mode;

   public:
	PMCCounter();
	explicit PMCCounter( CounterSet const& cset );
	void add( std::string counter_name );
	void add( CounterSet const& cset );
	void read() = 0;
	void stats() = 0;
	void start();
};
#endif // WITH_PMC

} // namespace pcnt

#endif // COUNTER_H_IN
