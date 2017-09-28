file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable( chain_test ${UNIT_TESTS} ${COMMON_SOURCES} )
target_link_libraries( chain_test eos_native_contract eos_chain chainbase eos_utilities eos_egenesis_none wallet_plugin fc ${PLATFORM_SPECIFIC_LIBS} )

if(WASM_TOOLCHAIN)
  file(GLOB SLOW_TESTS "slow_tests/*.cpp")
  add_executable( slow_test ${SLOW_TESTS} ${COMMON_SOURCES} )
  target_link_libraries( slow_test eos_native_contract eos_chain chainbase eos_utilities eos_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
  target_include_directories( slow_test PUBLIC ${CMAKE_BINARY_DIR}/contracts )
  add_dependencies(slow_test currency exchange)

  add_subdirectory(api_tests/memory_test)
  add_subdirectory(api_tests/extended_memory_test)
  file(GLOB API_TESTS "api_tests/*.cpp")
  add_executable( api_test ${API_TESTS} ${COMMON_SOURCES} )
  target_link_libraries( api_test eos_native_contract eos_chain chainbase eos_utilities eos_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
  target_include_directories( api_test PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_SOURCE_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/api_tests )
  add_dependencies(api_test test_api memory_test extended_memory_test)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosd_run_test.sh ${CMAKE_CURRENT_BINARY_DIR}/eosd_run_test.sh COPYONLY)
