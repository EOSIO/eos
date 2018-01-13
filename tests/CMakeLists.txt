#file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

set( CMAKE_CXX_STANDARD 14 )

include_directories("${CMAKE_BINARY_DIR}/tests/api_tests")
#include_directories("${CMAKE_BINARY_DIR}/contracts")
#include_directories("${CMAKE_SOURCE_DIR}/contracts")


#file(GLOB UNIT_TESTS "tests/*.cpp")
#if(WASM_TOOLCHAIN)
#  add_subdirectory(tests/contracts/rate_limit_auth)
#  file(GLOB WASM_UNIT_TESTS "tests/wasm_tests/*.cpp")
#  list(APPEND UNIT_TESTS ${WASM_UNIT_TESTS})
#endif()
#add_executable( chain_test ${UNIT_TESTS} ${COMMON_SOURCES} )
#target_link_libraries( chain_test eos_native_contract eosio_chain chainbase eos_utilities eos_egenesis_none wallet_plugin chain_plugin producer_plugin fc ${PLATFORM_SPECIFIC_LIBS} )
#if(WASM_TOOLCHAIN)
#  target_include_directories( chain_test PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/tests/contracts )
#  add_dependencies(chain_test rate_limit_auth)
#endif()


#file(GLOB UNIT_TESTS "chain_tests/*.cpp")
file(GLOB API_TESTS "api_tests/api_tests.cpp")
#add_custom_target(api_tests SOURCES "api_tests/api_tests.cpp")
if(WASM_TOOLCHAIN)
    file(GLOB WASM_UNIT_TESTS "wasm_tests/*.cpp")
endif()

#add_executable( unit_tests ${API_TESTS} ${UNIT_TESTS} ${WASM_TESTS} ${WASM_UNIT_TESTS} common/main.cpp )
add_executable( unit_tests ${API_TESTS} ${UNIT_TESTS} common/main.cpp )
target_link_libraries( unit_tests eosio_testing eosio_chain chainbase eos_utilities eos_egenesis_none chain_plugin fc ${PLATFORM_SPECIFIC_LIBS} )

if(WASM_TOOLCHAIN)
  target_include_directories( unit_tests PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/tests/contracts )
  add_dependencies(asserter test_api currency proxy)
endif()


###########################################################################################
#
# Setup for the `api_tests`
#
###########################################################################################
#"api_tests/db_test/db_test.cpp" 
add_subdirectory(api_tests)
file(GLOB API_SC_TESTS "api_tests/extended_memory_test/extended_memory_test.cpp" 
							  "api_tests/action_test/action_test.cpp" 
							  "api_tests/account_test/account_test.cpp" 
							  "api_tests/chain_test/chain_test.cpp" 
							  "api_tests/crypto_test/crypto_test.cpp" 
							  "api_tests/fixedpoint_test/fixedpoint_test.cpp" 
							  "api_tests/math_test/math_test.cpp" 
							  "api_tests/print_test/print_test.cpp" 
							  "api_tests/string_test/string_test.cpp" 
							  "api_tests/types_test/types_test.cpp")
# we need to get the `custom` target name that wasm.cmake will generate with add_wast_target
foreach(api_sc_test_f ${API_SC_TESTS})
	get_filename_component(_tmp ${api_sc_test_f} NAME)	
	string(REGEX REPLACE ".cpp" "" _tmp ${_tmp})
	add_dependencies(unit_tests "${_tmp}") # add the add_wast_target target `${_tmp}.sc`
endforeach(api_test_f)

###########################################################################################
#
# Setup for building the entire `unit_tests` binary
#
###########################################################################################


# WASM only tests
#if(WASM_TOOLCHAIN)
#  file(GLOB SLOW_TESTS "slow_tests/*.cpp")
#  add_executable( slow_test ${SLOW_TESTS} ${COMMON_SOURCES} )
#  target_link_libraries( slow_test eos_native_contract eosio_chain chainbase eos_utilities eos_egenesis_none chain_plugin producer_plugin fc ${PLATFORM_SPECIFIC_LIBS} )
#  target_include_directories( slow_test PUBLIC ${CMAKE_BINARY_DIR}/contracts )
#  add_dependencies(slow_test currency exchange)
#  add_subdirectory(api_tests/memory_test)
#  add_subdirectory(api_tests/extended_memory_test)
#  add_subdirectory(api_tests/table_abi_test)
#  file(GLOB API_TESTS "api_tests/*.cpp")
#  add_executable( api_test ${API_TESTS} ${COMMON_SOURCES} )
#  target_link_libraries( api_test eos_native_contract eosio_chain chainbase eos_utilities eos_egenesis_none chain_plugin producer_plugin fc ${PLATFORM_SPECIFIC_LIBS} )
#  target_include_directories( api_test PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_SOURCE_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/api_tests )
#  add_dependencies(api_test test_api memory_test extended_memory_test table_abi_test)
#endif()
#
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosd_run_test.sh ${CMAKE_CURRENT_BINARY_DIR}/eosd_run_test.sh COPYONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosd_run_mongodb_test.sh ${CMAKE_CURRENT_BINARY_DIR}/eosd_run_mongodb_test.sh COPYONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/p2p_tests/sync/test.sh ${CMAKE_CURRENT_BINARY_DIR}/p2p_tests/sync/test.sh COPYONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh COPYONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/trans_sync_across_mixed_cluster_test.sh ${CMAKE_CURRENT_BINARY_DIR}/trans_sync_across_mixed_cluster_test.sh COPYONLY)

