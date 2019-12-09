#ifndef SHUFFLE_H_IN
#define SHUFFLE_H_IN

#include <algorithm>
#include <iterator>
#include <random>
#include <vector>

template<typename T> void shuffle_array( std::vector<T>& arr )
{
    std::vector<T> copied = arr;
    std::vector<int> candidates;
	std::random_device rd;
	std::mt19937 g( rd() );
    for(int i = 1; i < arr.size(); i++) {
        candidates.push_back(i);
    }
    int curr = 0;
    for(int i = 1; i < arr.size(); i++) {
        std::uniform_int_distribution<uint64_t> dist(0, candidates.size() - 1);
        int next = candidates[dist(g)];
        arr[curr] = copied[next];
        candidates.erase(std::remove(candidates.begin(), candidates.end(), next), candidates.end());
        curr = next;
    }
    arr[curr] = copied[0];
    /*
    for(int i = 0; i < arr.size(); i++) {
        if(std::find(copied.begin(), copied.end(), arr[i]) != copied.end())
            //std::cout<< i << "\t";
    }
    //std::cout << std::endl;
    for(int i = 0; i < arr.size(); i++) {
        if(std::find(copied.begin(), copied.end(), arr[i]) != copied.end())
            //std::cout<< std::find(copied.begin(), copied.end(), arr[i]) - copied.begin()<< "\t";
    }
    //std::cout << std::endl;
    */

}

void set_shuffled_linked_list(char* &arr, uint64_t size, uint64_t stride) 
{
	std::vector<char*> rndarray;
	for( uint64_t i = 0; i < size; i += stride )
	{
		rndarray.push_back( (char*)&arr[i] );
	}

	char** head        = (char**)arr;
	char** shared_iter = head;
	shuffle_array<char*>( rndarray );

	for( uint64_t i = 0; i < size; i += stride )
	{
		*shared_iter = *(char**)&rndarray[i / stride];

		shared_iter += ( stride / sizeof( shared_iter ) );
	}
	*shared_iter = (char*)head;
}
    


#endif // SHUFFLE_H_IN
