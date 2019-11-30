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
void allocateRandomArray( long double );
void accessArray( char*, long double, char** );

int main()
{
	// call the function for allocating arrays of increasing size in MB
	allocateRandomArray( .00049 );
	allocateRandomArray( .00098 );
	allocateRandomArray( .00195 );
	allocateRandomArray( .00293 );
	allocateRandomArray( .00391 );
	allocateRandomArray( .00586 );
	allocateRandomArray( .00781 );
	allocateRandomArray( .01172 );
	allocateRandomArray( .01562 );
	allocateRandomArray( .02344 );
	allocateRandomArray( .03125 );
	allocateRandomArray( .04688 );
	allocateRandomArray( .0625 );
	allocateRandomArray( .09375 );
	allocateRandomArray( .125 );
	allocateRandomArray( .1875 );
	allocateRandomArray( .25 );
	allocateRandomArray( .375 );
	allocateRandomArray( .5 );
	allocateRandomArray( .75 );
	allocateRandomArray( 1 );
	allocateRandomArray( 1.5 );
	allocateRandomArray( 2 );
	allocateRandomArray( 3 );
	allocateRandomArray( 4 );
	allocateRandomArray( 6 );
	allocateRandomArray( 8 );
	allocateRandomArray( 12 );
	allocateRandomArray( 16 );
	allocateRandomArray( 24 );
	allocateRandomArray( 32 );
	allocateRandomArray( 48 );
	allocateRandomArray( 64 );
	allocateRandomArray( 96 );
	allocateRandomArray( 128 );
	allocateRandomArray( 192 );
}

void allocateRandomArray( long double size )
{
	int accessSize = ( 1024 * 1024 * size ); // array size in bytes
	char* randomArray = malloc(
	    accessSize * sizeof( char ) ); // allocate array of size allocate size
	int counter;
	int strideSize = 4096; // step size

	char** head
	    = (char**)randomArray; // start of linked list in contiguous memory
	char** iterate = head;     // iterator for linked list
	for( counter = 0; counter < accessSize; counter += strideSize )
	{
		( *iterate )
		    = &randomArray[counter
		                   + strideSize]; // iterate through linked list, having
		                                  // each one point stride bytes forward
		iterate += ( strideSize
		             / sizeof(
		                 iterate ) ); // increment iterator stride bytes forward
	}
	*iterate = (char*)head; // set tailf to point to head

	accessArray( randomArray, size, head );
	free( randomArray );
}

void accessArray( char* cacheArray, long double size, char** head )
{
	const long double NUM_ACCESSES
	    = 1000000000 / 100;                // number of accesses to linked list
	const int SECONDS_PER_NS = 1000000000; // const for timer
	FILE* fp                 = fopen( "accessData.txt", "a" ); // open file for writing data
	int newIndex             = 0;
	int counter              = 0;
	int read                 = 0;
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
	fprintf( fp, "%Lf\t%Lf\n", accessTime, size ); // print results to file
	fclose( fp );                                  // close file
}
