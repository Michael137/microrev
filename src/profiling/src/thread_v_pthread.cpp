#include <pthread.h>
#include <chrono>
#include <iostream>
#include <thread>

#include "time_utils.h"

uint64_t spawn_run_join()
{
	uint64_t start = pcnt::rdtsc();

	std::thread t1( []() {
		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		return 0;
	} );

	t1.join();

	uint64_t end = pcnt::rdtsc();

	return end - start;
}

/*
 * Goal: std::thread vs. pthread performance
 *       under different use-cases.
 *
 * Hypothesis: there is negligible difference in performance
 *             between pthread and std::thread
 * Link to validate:
 * https://www.reddit.com/r/cpp/comments/4zoz9v/performance_of_pthread_vs_stdthread_on_unix_based/
 *
 * Use-cases:
 * 1. Passing arguments
 * 2. Starting at the same time
 * 3. Cancelling (account for future/promises)
 */
int main( int argc, char** argv )
{
	std::cout << "std::thread spawn_run_join: " << spawn_run_join() << '\n';

	std::cout << std::endl;
	return 0;
}
