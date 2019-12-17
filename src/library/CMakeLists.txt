project(mrev_lib VERSION 0.1 LANGUAGES CXX)

add_library(lib
	lib/counter.cpp
	lib/time_utils.cpp
	lib/thread_utils.cpp
	lib/benchmark_helpers.cpp
)

# Link any external libraries here
#set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR/cmake}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(PAPI REQUIRED)
include_directories(${PAPI_INCLUDE_DIRS})

target_link_libraries(lib
	Threads::Threads
	papi)

target_include_directories(lib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
	PRIVATE src)

#target_compile_features(lib
#	PUBLIC cxx_auto_type)

install(TARGETS lib EXPORT mrev_library_config
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Copy over header files into install directory
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_executable(a_latency test/a_latency.cpp)
target_link_libraries(a_latency lib)

add_executable(b_latency test/b_latency.cpp)
target_link_libraries(b_latency lib)

add_executable(c_latency test/c_latency.cpp)
target_link_libraries(c_latency lib)

add_executable(diff_core_diff_bench test/diff_core_diff_bench.cpp)
target_link_libraries(diff_core_diff_bench lib)

add_executable(priority_schedule test/priority_schedule.cpp)
target_link_libraries(priority_schedule lib)

add_executable(reader_writer test/reader_writer.cpp)
target_link_libraries(reader_writer lib)

install(TARGETS 
	a_latency b_latency c_latency diff_core_diff_bench priority_schedule reader_writer
	EXPORT mrev_tests_config
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/test)
