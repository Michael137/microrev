#ifndef SHUFFLE_H_IN
#define SHUFFLE_H_IN

#include <algorithm>
#include <iterator>
#include <random>
#include <vector>

template<typename T> void shuffle_array( std::vector<T>& arr )
{
	std::random_device rd;
	std::mt19937 g( rd() );
	std::shuffle( arr.begin(), arr.end(), g );
}

#endif // SHUFFLE_H_IN
