#include <thread>
#include <iostream>
#include <pthread.h>

int main(int argc, char** argv) {
	std::thread t1([](){ std::cout << "T1" << std::endl; });

	t1.join();
	return 0;
}
