#include <sysexits.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "constants.h"
#include "counter.h"

#if defined( __FreeBSD__ ) && defined( WITH_PMC )
#	include <pmc.h>
#	include <sysexits.h>
#elif defined( WITH_PAPI_HL ) || defined( WITH_PAPI_LL )
#	include <papi.h>
#endif

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
    : Counter<std::vector<pmc_value_t>, CounterSet>()
    , pmcid()
    , mode( PMC_MODE_TC )
{
}

PMCCounter::PMCCounter( CounterSet const& cset )
    : Counter<std::vector<pmc_value_t>, CounterSet>( cset )
    , pmcid()
    , mode( PMC_MODE_TC )
{
}

// PMC_MODE_TC: Per-processor counter
void PMCCounter::start()
{
	if( pmc_init() < 0 )
		err( EX_OSERR, "Cannot initialize pmc(3)" );

	if( this->cset.size() > pcnt::max_pmc_num )
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
	assert( this->cset.size() == this->measured.size() );
	for( int i = 0; i < this->cset.size(); ++i )
	{
		std::cout << this->cset[i] << ": " << this->measured[i] << '\n';
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
		this->measured.push_back( v );
	}
}

void PMCCounter::add( CounterSet const& cset )
{
	this->cset.insert( this->cset.end(), cset.begin(), cset.end() );
}
#elif defined( WITH_PAPI_HL ) // !WITH_PMC

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
#elif defined( WITH_PAPI_LL ) // !WITH_PAPI_HL
PAPILLCounter::PAPILLCounter( std::vector<int> cset )
    : Counter<std::vector<long_long>, std::vector<int>>( cset )
    , event_set()
{
	this->init();
}

PAPILLCounter::PAPILLCounter()
    : Counter<std::vector<long_long>, std::vector<int>>()
    , event_set()
{
	this->init();
}

void PAPILLCounter::add( std::string counter_name ) {}
void PAPILLCounter::add( std::vector<int> const& events )
{
	this->event_set = PAPI_NULL;
	int retval      = 0;
	if( ( retval = PAPI_register_thread() ) != PAPI_OK )
	{
		PAPI_perror( "PAPI couldn't register thread" );
		exit( 1 );
	}

	if( PAPI_create_eventset( &( this->event_set ) ) != PAPI_OK )
	{
		PAPI_perror( "failed to create eventset" );
		exit( 1 );
	}

	this->cset.insert( this->cset.end(), events.begin(), events.end() );
	if( PAPI_add_events( this->event_set, const_cast<int*>( events.data() ),
	                     events.size() )
	    != PAPI_OK )
	{
		PAPI_perror( "failed to add events" );
		exit( 1 );
	}
}
void PAPILLCounter::read()
{
	this->measured.resize( this->cset.size() );
	if( PAPI_stop( this->event_set, this->measured.data() ) != PAPI_OK )
	{
		PAPI_perror( "failed to read counters" );
		exit( 1 );
	}

	int retval = 0;
	if( ( retval = PAPI_cleanup_eventset( this->event_set ) ) != PAPI_OK )
	{
		PAPI_perror( "failed to clean up eventset" );
		exit( 1 );
	}

	if( ( retval = PAPI_unregister_thread() ) != PAPI_OK )
	{
		PAPI_perror( "failed to unregister thread" );
		exit( 1 );
	}
}

void PAPILLCounter::stats()
{
	assert( this->cset.size() == this->measured.size() );
	char name[PAPI_MAX_STR_LEN];
	for( int i = 0; i < this->cset.size(); ++i )
	{
		PAPI_event_code_to_name(cset[i], name);
		std::cout << name << ": " << this->measured[i] << '\n';
	}
	std::cout << std::endl;
}

void PAPILLCounter::start()
{
	if( PAPI_start( this->event_set ) != PAPI_OK )
	{
		PAPI_perror( "failed to start counters" );
		exit( 1 );
	}
}

unsigned long current_thread_id()
{
	std::stringstream id_stream;
	id_stream << std::this_thread::get_id();
	unsigned long id;
	id_stream >> id;
	return id;
}

void PAPILLCounter::init()
{
	int retval = PAPI_library_init( PAPI_VER_CURRENT );
	if( retval != PAPI_VER_CURRENT )
	{
		PAPI_perror( "PAPI library init error!" );
		exit( 1 );
	}

	if( PAPI_thread_init( current_thread_id ) != PAPI_OK )
	{
		PAPI_perror( "PAPI thread library init error!" );
		exit( 1 );
	}
}
#endif                        // !WITH_PAPI_LL

} // namespace pcnt
