#ifndef COUNTER_H_IN
#define COUNTER_H_IN

#include <err.h>
#include <future>
#include <string>
#include <vector>

#if defined( __FreeBSD__ ) && defined( WITH_PMC )
#	include <pmc.h>
#	include <sysexits.h>
#elif defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

#include "counters.h"

namespace pcnt
{
// List of counters: `pmccontrol -L`
template<typename ResultVec, typename EventsVec = CounterSet> class Counter
{
   protected:
	ResultVec measured;

   public:
	EventsVec cset;
	double cycles_measured;
	std::vector<int64_t> vec_cycles_measured;
	int core_id;
	bool collect;

   public:
	Counter();
	virtual ~Counter() {}
	explicit Counter( EventsVec const& cset );
	virtual void add( std::string counter_name ) = 0;
	virtual void add( EventsVec const& cset )    = 0;
	virtual void read()                          = 0;
	virtual void stats()                         = 0;
	virtual void start()                         = 0;
	size_t num_events() { return cset.size(); }
	void set_cycles_measured( uint64_t c ) { this->cycles_measured = c; }
};

#ifdef WITH_PMC
class PMCCounter : public Counter<std::vector<pmc_value_t>, CounterSet>
{
   private:
	pmc_id_t pmcid;
	enum pmc_mode mode;

   public:
	PMCCounter();
	explicit PMCCounter( CounterSet const& cset );
	void add( std::string counter_name );
	void add( CounterSet const& cset );
	void read();
	void stats();
	void start();
};
#elif defined( WITH_PAPI_LL ) // !WITH_PMC
// NOTE: run `papi_avail -a` to get details of PAPI counters that are supported
// on HW
// NOTE: run `papi_native_avail` to get a more complete list of events supported
// by chip
class PAPILLCounter : public Counter<std::vector<long_long>, std::vector<int>>
{
   private:
	int event_set;
	int max_hw_cntrs;

   public:
	PAPILLCounter();
	~PAPILLCounter();
	explicit PAPILLCounter( std::vector<int> cset );

	void add( std::string counter_name ) {}
	void add( std::vector<int> const& cset ) {
	} // TODO: event_set could be std::string; then don't need this empty
	  // virtual method impl
	void add( std::vector<std::string> const& cset );
	void read();
	void stats();
	void start();
	void init();
	void end();
	void reset();
};
#endif                        // WITH_PAPI_LL

} // namespace pcnt

#endif // COUNTER_H_IN
