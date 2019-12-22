#include <pthread.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "counter.h"

#define INST_INIT() int a = 1, b = 1, c = 1;

#define INST_BODY()                                                            \
	a = b + 1;                                                                 \
	b = c + 1;                                                                 \
	c = b + 1;

#define R5( a ) a a a a a
#define R20( a ) R5( a ) R5( a ) R5( a ) R5( a )
#define R25( a ) R20( a ) R5( a )
#define R100( a ) R25( a ) R25( a ) R25( a ) R25( a )
#define R500( a ) R100( a ) R100( a ) R100( a ) R100( a ) R100( a )
#define R1024( a ) R500( a ) R500( a ) R20( a ) a
#define R2048( a ) R1024( a ) R1024( a )
#define R4096( a ) R2048( a ) R2048( a )
#define R8192( a ) R4096( a ) R4096( a )
#define R16384( a ) R8192( a ) R8192( a )
#define R32768( a ) R16384( a ) R16384( a )
#define R65536( a ) R32768( a ) R32768( a )

#define BENCH_FN( size )                                                       \
	void __attribute__( ( optimize( "0" ) ) ) benchmark##size()                \
	{                                                                          \
		INST_INIT();                                                           \
		R##size( INST_BODY() );                                                \
	}

BENCH_FN( 4096 );
BENCH_FN( 8192 );
BENCH_FN( 16384 );
BENCH_FN( 32768 );
BENCH_FN( 65536 );

/*
 * Goal: Reproduce results from "Just How Accurate Are Performance Counters?" by
 * Korn et al.
 *
 * Hypothesis: Assuming 4-byte instructions and a 32 KB I-cache,
 *             an instruction footprint of > 8192 should cause
 *             I-cache misses
 */
int main( int argc, char** argv )
{
	using namespace pcnt;

	PAPILLCounter pc;

	std::vector<std::string> events;
	events.push_back( "PAPI_L1_ICM" );

	pc.add( events );

	pc.reset();
	pc.start();
	benchmark4096();
	pc.read();
	pc.print_stats();

	pc.reset();
	pc.start();
	benchmark8192();
	pc.read();
	pc.print_stats();

	pc.reset();
	pc.start();
	benchmark16384();
	pc.read();
	pc.print_stats();

	pc.reset();
	pc.start();
	benchmark32768();
	pc.read();
	pc.print_stats();

	pc.reset();
	pc.start();
	benchmark65536();
	pc.read();
	pc.print_stats();

	pc.end();

	return 0;
}
