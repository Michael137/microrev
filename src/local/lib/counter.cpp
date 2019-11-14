#include <sysexits.h>
#include <cassert>
#include <iostream>
#include <string>

#include "constants.h"
#include "counter.h"

namespace pcnt
{

template<typename CntResult>
Counter<CntResult>::Counter()
    : cset()
    , measured()
{
}

template<typename CntResult>
Counter<CntResult>::Counter( CounterSet const& cset )
    : cset( cset )
    , measured()
{
}

#ifdef WITH_PMC
PMCCounter::Counter()
    : Counter<pmc_value_t>()
    , pmcid()
    , mode( PMC_MODE_TC )
{
}

PMCCounter::Counter( CounterSet const& cset )
    : Counter<pmc_value_t>( cset )
    , pmcid()
    , mode( PMC_MODE_TC )
{
}

// PMC_MODE_TC: Per-processor counter
void PMCCounter::start()
{
	if( pmc_init() < 0 )
		err( EX_OSERR, "Cannot initialize pmc(3)" );

	if( this->cset.size() > pmc_utils::max_pmc_num )
		err( EX_OSERR, "Too many pmc counters specified" );

	for( auto& ctr: this->cset )
	{
		if( pmc_allocate( ctr.c_str(), mode, 0, PMC_CPU_ANY, &( this->pmcid ),
		                  0 )
		    < 0 )
		{
			err( EX_OSERR, "Cannot allocate counter \"%s\"", ctr.c_str() );
		}
	}

	if( pmc_start( this->pmcid ) < 0 )
		err( EX_OSERR, "Cannot start pmc" );
}

void PMCCounter::stats()
{
	assert( this->cset.size() == this->pmc_values.size() );
	for( int i = 0; i < this->cset.size(); ++i )
	{
		std::cout << this->cset[i] << ": " << this->pmc_values[i] << '\n';
	}
	std::cout << std::endl;
}

void PMCCounter::add( std::string counter_name )
{
	this->cset.push_back( counter_name );
}

void PMCCounter::read()
{
	uint64_t v;
	for( auto& e: this->cset )
	{
		if( pmc_read( this->pmcid, &v ) < 0 )
			err( EX_OSERR, "Cannot read pmc" );
		this->pmc_values.push_back( v );
	}
}

void PMCCounter::add( CounterSet const& cset )
{
	this->cset.insert( this->cset.end(), cset.begin(), cset.end() );
}
#endif // WITH_PMC

} // namespace pcnt
