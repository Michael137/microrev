#include <random>
#include <algorithm>
#include <iterator>

void shuffle(char* arr, uint64_t size) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(arr, arr + size, g);
}
