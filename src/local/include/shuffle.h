#include <random>
#include <algorithm>
#include <iterator>
#include <vector>

template<typename T>
void myshuffle(std::vector<T>&arr) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(arr.begin(), arr.end(), g);
}
