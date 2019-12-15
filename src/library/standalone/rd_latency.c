#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// MACROS
#define ONE iterate = (char**)*iterate;
#define FIVE ONE ONE ONE
#define TWOFIVE FIVE FIVE FIVE FIVE FIVE
#define HUNDO TWOFIVE TWOFIVE TWOFIVE TWOFIVE

// prototype
void allocateRandomArray( long long );
void accessArray( char*, long long, char** );

int main()
{
	// call the function for allocating arrays of increasing size in MiB
	for( long long i = 512; i < 268435456; i *= 2 )
		allocateRandomArray( i );
}

// Pre-condition: stride < size aligned to nearest power of 2
// If size < 8192 => size should be a power of 2
void allocateRandomArray( long long size )
{
	char* randomArray = malloc(
	    size * sizeof( char ) ); // allocate array of size allocate size

	int stride = 64;

	long long i;
	for( i = stride; i < size; i += stride )
	{
		// Arrange linked list such that:
		// Pointer at arr[i] == Pointer at arr[i + stride]
		*(char**)&randomArray[i - stride] = (char*)&randomArray[i];
	}

	// Loop back end of linked list to the head
	*(char**)&randomArray[i - stride] = (char*)&randomArray[0];

	char** head = (char**) randomArray;
	accessArray( randomArray, size, head );
	free( randomArray );
}

void accessArray( char* cacheArray, long long size, char** head )
{
	const long double NUM_ACCESSES
	    = 1000000000 / 100;                // number of accesses to linked list
	const int SECONDS_PER_NS = 1000000000; // const for timer
	FILE* fp     = fopen( "accessData.txt", "a" ); // open file for writing data
	int newIndex = 0;
	int counter  = 0;
	int read     = 0;
	struct timespec startAccess, endAccess; // struct for timer
	long double accessTime = 0;
	char** iterate         = head; // create iterator

	clock_gettime( CLOCK_REALTIME, &startAccess ); // start clock
	for( counter = 0; counter < NUM_ACCESSES; counter++ )
	{
		HUNDO // macro subsitute 100 accesses to mitigate loop overhead
	}
	clock_gettime( CLOCK_REALTIME, &endAccess ); // end clock
	// calculate the time elapsed in ns per access
	accessTime
	    = ( ( ( endAccess.tv_sec - startAccess.tv_sec ) * SECONDS_PER_NS )
	        + ( endAccess.tv_nsec - startAccess.tv_nsec ) )
	      / ( 100 * NUM_ACCESSES );
	fprintf( fp, "%Lf\t%lld\n", accessTime, size ); // print results to file
	fclose( fp );                                  // close file
}
