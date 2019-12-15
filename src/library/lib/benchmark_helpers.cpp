#include <inttypes.h>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

#include "benchmark_helpers.h"

// From:
// https://codereview.stackexchange.com/questions/169880/computing-average-without-overflows-and-with-half-decent-precision
uint64_t avg_no_overflow( std::vector<uint64_t> const& nums )
{
	auto first = nums.begin();
	auto last  = nums.end();

	const auto size = std::distance( first, last );
	if( !size )
		return 0;

	std::intmax_t accumulator = 0;
	long double res           = 0;
	do
	{
		using snl = std::numeric_limits<decltype( accumulator )>;
		if( accumulator < 0 && snl::min() - accumulator > *first )
		{
			res += accumulator / size - 1;
			accumulator = accumulator % size + size;
		}
		else if( accumulator > 0 && snl::max() - accumulator < *first )
		{
			res += accumulator / size + 1;
			accumulator = accumulator % size - size;
		}
		accumulator += *first;
	} while( ++first != last );
	res += accumulator / size;
	res += static_cast<decltype( res )>( accumulator % size ) / size;
	return res;
}
