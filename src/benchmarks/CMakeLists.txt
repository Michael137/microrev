add_executable(cache src/cache.cpp)
target_link_libraries(cache lib)

add_executable(flush src/flush.cpp)
target_link_libraries(flush lib)

add_executable(producer_consumer src/producer_consumer.cpp)
target_link_libraries(producer_consumer lib)

add_executable(protocols src/protocols.cpp)
target_link_libraries(protocols lib)

add_executable(cacheline src/cacheline.cpp)
target_link_libraries(cacheline lib)

add_executable(rd_latency src/rd_latency.cpp)
target_link_libraries(rd_latency lib)

install(TARGETS 
	cache flush producer_consumer protocols cacheline rd_latency
	EXPORT mrev_benchmarks_config
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmarks)
