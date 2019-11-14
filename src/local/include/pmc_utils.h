#ifndef PMC_H_IN
#define PMC_H_IN

#include <string>
#include <vector>

#include <err.h>
#include <pmc.h>
#include <sysexits.h>

namespace pmc_utils
{
using CounterSet = std::vector<std::string>;
using PMCValues  = std::vector<pmc_value_t>;

// List of counters: `pmccontrol -L`
class Counter
{
   private:
	CounterSet cset;
	PMCValues pmc_values;
	pmc_id_t pmcid;
	enum pmc_mode mode;

   public:
	Counter();
	void add( std::string counter_name );
	void read();
	void stats();
	void start();
};

void pmc_begin( Counter& counter );
void pmc_end( Counter& counter );
void cset_push( Counter& counter );
void cset_from_config( Counter& counter );

} // namespace pmc_utils

#endif // PMC_H_IN
