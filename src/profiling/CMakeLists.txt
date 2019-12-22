add_executable(thread_v_pthread src/thread_v_pthread.cpp)
target_link_libraries(thread_v_pthread lib)

add_executable(icache_linear src/icache_linear.cpp)
target_link_libraries(icache_linear lib)

install(TARGETS 
	thread_v_pthread
	icache_linear
	EXPORT mrev_profiling_config
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/profiling)
