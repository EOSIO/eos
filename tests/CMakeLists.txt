#file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

set( CMAKE_CXX_STANDARD 14 )

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



file(GLOB UNIT_TESTS "chain_tests/*.cpp" "tests/abi_tests.cpp" "tests/database_tests.cpp")
if(WASM_TOOLCHAIN)
    file(GLOB WASM_UNIT_TESTS "wasm_tests/*.cpp")
endif()

add_executable( chain_test ${UNIT_TESTS} ${WASM_UNIT_TESTS} common/main.cpp )
target_link_libraries( chain_test eosio_testing eosio_chain chainbase eos_utilities eos_egenesis_none chain_plugin abi_generator fc ${PLATFORM_SPECIFIC_LIBS} )

if(WASM_TOOLCHAIN)
  target_include_directories( chain_test PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/tests/contracts )
  target_include_directories( chain_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wasm_tests )
  add_dependencies(chain_test asserter test_api currency proxy identity stltest infinite)
endif()


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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/p2p_tests/sync/test.sh ${CMAKE_CURRENT_BINARY_DIR}/p2p_tests/sync/test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/p2p_tests/dawn_515/test.sh ${CMAKE_CURRENT_BINARY_DIR}/p2p_tests/dawn_515/test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/trans_sync_across_mixed_cluster_test.sh ${CMAKE_CURRENT_BINARY_DIR}/trans_sync_across_mixed_cluster_test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-remote-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-remote-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sample-cluster-map.json ${CMAKE_CURRENT_BINARY_DIR}/sample-cluster-map.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/restart-scenarios-test.py ${CMAKE_CURRENT_BINARY_DIR}/restart-scenarios-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testUtils.py ${CMAKE_CURRENT_BINARY_DIR}/testUtils.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosiod_run_test.py ${CMAKE_CURRENT_BINARY_DIR}/eosiod_run_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosiod_run_remote_test.py ${CMAKE_CURRENT_BINARY_DIR}/eosiod_run_remote_test.py COPYONLY)

add_test(chain_test chain_test --report_level=detailed)
add_test(NAME eosiod_run_test COMMAND tests/eosiod_run_test.py -v --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME eosiod_run_remote_test COMMAND tests/eosiod_run_remote_test.py --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME p2p_dawn515_test COMMAND tests/p2p_tests/dawn_515/test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: Tests removed until working again on master.
# TODO: add_test(NAME p2p_sync_test COMMAND tests/p2p_tests/sync/test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME p2p_sync_test_p2_d10 COMMAND tests/p2p_tests/sync/test.sh -p 2 -d 10 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME message_storm COMMAND tests/p2p_tests/sync/test.sh -m -p 21 -n 21 -d 5 -l WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: if(BUILD_MONGO_DB_PLUGIN)
# TODO:   add_test(NAME eosiod_run_test-mongodb COMMAND tests/eosiod_run_test.py --mongodb --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: endif()
# TODO: add_test(NAME trans_sync_across_mixed_cluster_test COMMAND tests/trans_sync_across_mixed_cluster_test.sh -p 1 -n 2 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME distributed-transactions-test COMMAND tests/distributed-transactions-test.py -p 1 -n 4 --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME distributed-transactions-remote-test COMMAND tests/distributed-transactions-remote-test.py --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME restart-scenarios-test_resync COMMAND tests/restart-scenarios-test.py -c resync -p3 --dumpErrorDetails WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME restart-scenarios-test_replay COMMAND tests/restart-scenarios-test.py -c replay -p3 --dumpErrorDetails WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

if(ENABLE_COVERAGE_TESTING)

  set(Coverage_NAME ${PROJECT_NAME}_coverage)
  set(Coverage_EXECUTABLE chain_test)

  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif() # NOT GENHTML_PATH

  # Setup target
  add_custom_target(${Coverage_NAME}

    # Cleanup lcov
    COMMAND ${LCOV_PATH} --directory . --zerocounters
    # Create baseline to make sure untouched files show up in the report
    COMMAND ${LCOV_PATH} -c -i -d . -o ${Coverage_NAME}.base

    # Run tests
    COMMAND ${Coverage_EXECUTABLE}

    # Capturing lcov counters and generating report
    COMMAND ${LCOV_PATH} --directory . --capture --output-file ${Coverage_NAME}.info
    # add baseline counters
    COMMAND ${LCOV_PATH} -a ${Coverage_NAME}.base -a ${Coverage_NAME}.info --output-file ${Coverage_NAME}.total
    COMMAND ${LCOV_PATH} --remove ${Coverage_NAME}.total ${COVERAGE_EXCLUDES} --output-file ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
    COMMAND ${GENHTML_PATH} -o ${Coverage_NAME} ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
    COMMAND ${CMAKE_COMMAND} -E remove ${Coverage_NAME}.base ${Coverage_NAME}.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned

    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
    )

  # Show info where to find the report
  add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
    COMMAND ;
    COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
    )
endif()
