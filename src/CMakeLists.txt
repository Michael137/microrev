cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(mrev VERSION 0.1 LANGUAGES CXX)

include(GNUInstallDirs)

# Any external imports go here

# Our targets
enable_testing()

option(WITH_PAPI_LL "With PAPI low-level API" ON)

if(WITH_PAPI_LL)
	add_definitions(-DWITH_PAPI_LL)
endif(WITH_PAPI_LL)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/home/gardei/papi/install")

add_subdirectory(library)
#add_subdirectory(benchmarks)
