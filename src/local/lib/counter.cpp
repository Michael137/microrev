#include <sysexits.h>
#include <cassert>
#include <iostream>
#include <string>

#include "constants.h"
#include "counter.h"

#include <papi.h>

namespace pcnt
{
template<typename CntResult, typename Events>
Counter<CntResult, Events>::Counter()
    : cset()
    , measured()
{
}

template<typename CntResult, typename Events>
Counter<CntResult, Events>::Counter( Events const& cset )
    : cset( cset )
    , measured()
{
}

#ifdef WITH_PMC
PMCCounter::PMCCounter()
    : Counter<std::vector<pmc_value_t>>()
    , pmcid()
    , mode( PMC_MODE_TC )
{
}

PMCCounter::PMCCounter( CounterSet const& cset )
    : Counter<std::vector<pmc_value_t>>( cset )
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

PAPIHLCounter::PAPIHLCounter( std::vector<int> const& cset )
    : Counter<std::vector<long_long>, std::vector<int>>( cset )
{
}

PAPIHLCounter::PAPIHLCounter()
    : Counter<std::vector<long_long>, std::vector<int>>()
{
}

void PAPIHLCounter::add( std::string counter_name ) {}
void PAPIHLCounter::add( std::vector<int> const& cset ) {}
void PAPIHLCounter::read()
{
	this->measured.resize( this->cset.size() );
	if( PAPI_stop_counters( this->measured.data(), this->cset.size() )
	    != PAPI_OK )
	{
		PAPI_perror( "failed to read counters" );
		exit( 1 );
	}
}

void PAPIHLCounter::stats()
{
	assert( this->cset.size() == this->measured.size() );
	for( int i = 0; i < this->cset.size(); ++i )
	{
		std::cout << this->cset[i] << ": " << this->measured[i] << '\n';
	}
	std::cout << std::endl;
}

void PAPIHLCounter::start()
{
	// check for return of init
	PAPI_library_init( PAPI_VER_CURRENT );
	// check perf counter number supported by HW
	if( PAPI_start_counters( this->cset.data(), this->cset.size() ) != PAPI_OK )
	{
		PAPI_perror( "failed to start counters" );
		exit( 1 );
	}
}

} // namespace pcnt
