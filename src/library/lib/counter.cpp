#include <sysexits.h>
#include <cassert>
#include <fstream>
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
    , core_id()
    , cycles_measured()
    , vec_cycles_measured()
    , label()
{
}

template<typename CntResult, typename Events>
Counter<CntResult, Events>::Counter( Events const& cset )
    : cset( cset )
    , measured()
    , core_id()
    , cycles_measured()
    , vec_cycles_measured()
    , label()
{
}

template<typename CntResult, typename Events>
Counter<CntResult, Events>::Counter( std::string const& label )
    : cset()
    , measured()
    , core_id()
    , cycles_measured()
    , vec_cycles_measured()
    , label( label )
{
}

#ifdef WITH_PMC
PMCCounter::PMCCounter()
    : Counter<std::vector<pmc_value_t>, CounterSet>()
    , pmcid()
    , mode( PMC_MODE_SC )
{
}

PMCCounter::PMCCounter( CounterSet const& cset )
    : Counter<std::vector<pmc_value_t>, CounterSet>( cset )
    , pmcid()
    , mode( PMC_MODE_SC )
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
		if( pmc_allocate( ctr.c_str(), mode, 0, this->core_id, &( this->pmcid ),
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

	pmc_release( this->pmcid );
}

void PMCCounter::add( CounterSet const& cset )
{
	this->cset.insert( this->cset.end(), cset.begin(), cset.end() );
}
#elif defined( WITH_PAPI_LL ) // !WITH_PMC
inline void exit_with_err( std::string msg, int exit_code = 1 )
{
	PAPI_perror( const_cast<char*>( msg.c_str() ) );
	exit( exit_code );
}

PAPILLCounter::PAPILLCounter( std::vector<int> cset )
    : Counter<std::vector<long_long>, std::vector<int>>( cset )
    , event_set()
    , max_hw_cntrs()
{
	this->init();
}

PAPILLCounter::PAPILLCounter()
    : Counter<std::vector<long_long>, std::vector<int>>()
    , event_set()
    , max_hw_cntrs()
{
	this->init();
}

PAPILLCounter::~PAPILLCounter() {}

void PAPILLCounter::add( std::vector<std::string> const& events )
{
	this->event_set = PAPI_NULL;
	int retval      = 0;

	if( events.size() > this->max_hw_cntrs )
		exit_with_err( "Number of events exceeds available HW counters" );

	if( ( retval = PAPI_register_thread() ) != PAPI_OK )
		exit_with_err( "PAPI couldn't register thread" );

	if( PAPI_create_eventset( &( this->event_set ) ) != PAPI_OK )
		exit_with_err( "failed to create eventset" );

	if( events.size() > 0 )
	{
		std::vector<int> codes;
		for( int i = 0; i < events.size(); ++i )
		{
			int code = 0;
			retval   = PAPI_event_name_to_code(
                const_cast<char*>( events[i].c_str() ), &code );
			if( retval != PAPI_OK ){
				std::stringstream ss;
				ss << "couldn't convert event name to code\n";
				for( auto& e: events )
					ss << '\t' << e << std::endl;
				exit_with_err(ss.str().c_str() );
			}

			codes.push_back( code );
		}
		assert( codes.size() == events.size() );

		this->cset.insert( this->cset.end(), codes.begin(), codes.end() );
		if( PAPI_add_events( this->event_set, const_cast<int*>( codes.data() ),
		                     codes.size() )
		    != PAPI_OK )
		{
			std::stringstream ss;
			ss << "failed to add events:\n";
			for( auto& e: events )
				ss << '\t' << e << std::endl;

			exit_with_err( ss.str().c_str() );
		}
	}
}
void PAPILLCounter::read()
{
	if( this->cset.size() > 0 )
	{
		this->measured.resize( this->cset.size() );
		if( PAPI_stop( this->event_set, this->measured.data() ) != PAPI_OK )
			exit_with_err( "failed to read counters" );
	}
}

void PAPILLCounter::reset()
{
	if( this->cset.size() > 0 )
	{
		if( PAPI_reset( this->event_set ) != PAPI_OK )
			exit_with_err( "failed to reset counters" );
		this->measured.clear();
        this->vec_cycles_measured.clear();
		this->measured.resize( this->cset.size() );
	}
}

void PAPILLCounter::end()
{
	int retval = 0;
	if( ( retval = PAPI_cleanup_eventset( this->event_set ) ) != PAPI_OK )
		exit_with_err( "failed to clean up eventset" );

	if( ( retval = PAPI_destroy_eventset( &( this->event_set ) ) ) != PAPI_OK )
		exit_with_err( "failed to destroy eventset" );

	// TODO: should be moved to destructor
	if( ( retval = PAPI_unregister_thread() ) != PAPI_OK )
		exit_with_err( "failed to unregister thread" );
}

void PAPILLCounter::stats_to_stream( std::ostream& os )
{
	assert( this->cset.size() == this->measured.size() );
	char name[PAPI_MAX_STR_LEN];
	if( this->cset.size() == 0 && this->vec_cycles_measured.size() == 0 )
		return;

	// Note: when changing label layout also change scripts/parser.py
    if(!(this->label.empty()))
	    os << "~~" << this->label << "~~" << '\n';
	for( int i = 0; i < this->cset.size(); ++i )
	{
		PAPI_event_code_to_name( cset[i], name );
		os << name << ": " << this->measured[i] << '\n';
	}
	for( auto c: this->vec_cycles_measured )
	{
		this->cycles_measured
		    += ( (double)c ) / this->vec_cycles_measured.size();
	}
	os << "Cycles: " << this->cycles_measured << '\n';

	os << std::endl;
}

void PAPILLCounter::stats()
{
	std::ofstream ofs( "dump.dat", std::ofstream::out | std::ofstream::app );
	this->stats_to_stream( ofs );
	ofs.close();
}

void PAPILLCounter::print_stats() { this->stats_to_stream( std::cout ); }

void PAPILLCounter::start()
{
	if( this->cset.size() > 0 )
	{
		if( PAPI_start( this->event_set ) != PAPI_OK )
			exit_with_err( "failed to start counters" );
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
		exit_with_err( "PAPI library init error!" );

	if( ( this->max_hw_cntrs = PAPI_num_counters() ) == 0 )
		exit_with_err(
		    "Info:: This machine does not provide hardware counters." );

	if( PAPI_thread_init( current_thread_id ) != PAPI_OK )
		exit_with_err( "PAPI thread library init error!" );
}
#endif                        // !WITH_PAPI_LL

} // namespace pcnt
