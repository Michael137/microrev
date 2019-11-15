#ifndef COUNTER_H_IN
#define COUNTER_H_IN

#include <err.h>
#include <string>
#include <vector>

#if defined( __FreeBSD__ ) && defined( WITH_PMC )
#	include <pmc.h>
#	include <sysexits.h>
#else
#	include <papi.h>
#endif

#include "counters.h"

namespace pcnt
{
// List of counters: `pmccontrol -L`
template<typename ResultVec, typename EventsVec = CounterSet> class Counter
{
   protected:
	EventsVec cset;
	ResultVec measured;

   public:
	Counter();
	explicit Counter( EventsVec const& cset );
	virtual void add( std::string counter_name ) = 0;
	virtual void add( EventsVec const& cset )    = 0;
	virtual void read()                          = 0;
	virtual void stats()                         = 0;
	virtual void start()                         = 0;
};

#ifdef WITH_PMC
class PMCCounter : public Counter<std::vector<pmc_value_t>>
{
   private:
	pmc_id_t pmcid;
	enum pmc_mode mode;

   public:
	PMCCounter();
	explicit PMCCounter( CounterSet const& cset );
	void add( std::string counter_name );
	void add( CounterSet const& cset );
	void read()  = 0;
	void stats() = 0;
	void start();
};
#endif // WITH_PMC

class PAPIHLCounter : public Counter<std::vector<long_long>, std::vector<int>>
{
   private:
   public:
	PAPIHLCounter();
	explicit PAPIHLCounter( std::vector<int> const& cset );
	void add( std::string counter_name );
	void add( std::vector<int> const& cset );
	void read();
	void stats();
	void start();
};

} // namespace pcnt

#endif // COUNTER_H_IN
