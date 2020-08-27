/*
 * From U. Drepper 2007
 * Test likely/unlikely assumptions
 *
 * Compile using GCC and GNU assembler
 *
 * gcc branch_predictor.c -DDEBUGPRED
 *
 * Depending on GCC configuration need to add the "-no-pie" flag
 */

#include <stdio.h>

#ifndef DEBUGPRED
#	define unlikely( expr ) __builtin_expect( !!( expr ), 0 )
#	define likely( expr ) __builtin_expect( !!( expr ), 1 )
#else
#	ifdef __x86_64__
#		define debugpred__( e, E )                                            \
			( {                                                                \
				long int _e = !!( e );                                         \
				asm volatile(                                                  \
				    ".pushsection predict_data\n"                              \
				    "..predictcnt%=: .quad 0; .quad 0\n"                       \
				    ".section predict_line; .quad %c1\n"                       \
				    ".section predict_file; .quad %c2; .popsection\n"          \
				    "addq $1,..predictcnt%=(,%0,8)" ::"r"( _e == E ),          \
				    "i"( __LINE__ ), "i"( __FILE__ ) );                        \
				__builtin_expect( _e, E );                                     \
			} )
#		define unlikely( expr ) debugpred__( ( expr ), 0 )
#		define likely( expr ) debugpred__( ( expr ), 1 )
#	else
#		error "You're not on x86_64"
#	endif
#endif

extern long int __start_predict_data;
extern long int __stop_predict_data;
extern long int __start_predict_line;
extern const char* __start_predict_file;

static void __attribute__( ( destructor ) ) predprint( void )
{
	puts( "Branch prediction summary:" );
	long int* s     = &__start_predict_data;
	long int* e     = &__stop_predict_data;
	long int* sl    = &__start_predict_line;
	const char** sf = &__start_predict_file;

	while( s < e )
	{
		printf( "%s:%ld: incorrect=%ld, correct=%ld%s\n", *sf, *sl, s[0], s[1],
		        s[0] > s[1] ? "    <==== WARNING" : "" );
		++sl;
		++sf;
		s += 2;
	}
}

int main()
{
	// Create sections s.t. linker finds them when calling __start_XYZ and
	// __stop_XYZ
	asm( ".section predict_data, \"aw\"; .previous\n"
	     ".section predict_line, \"a\"; .previous\n"
	     ".section predict_file, \"a\"; .previous" );

	int i = 0;
	while( likely( i++ < 1e6 ) ) {}

	return 0;
}
