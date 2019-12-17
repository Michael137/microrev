add_executable(thread_v_pthread src/thread_v_pthread.cpp)
target_link_libraries(thread_v_pthread lib)

install(TARGETS 
	thread_v_pthread
	EXPORT mrev_profiling_config
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/profiling)
