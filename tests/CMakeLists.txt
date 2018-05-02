
find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

set( CMAKE_CXX_STANDARD 14 )

include_directories("${CMAKE_SOURCE_DIR}/plugins/wallet_plugin/include")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp ESCAPE_QUOTES)

file(GLOB UNIT_TESTS "wallet_tests.cpp")

add_executable( plugin_test ${UNIT_TESTS} ${WASM_UNIT_TESTS} main.cpp)
target_link_libraries( plugin_test eosio_testing eosio_chain chainbase eos_utilities chain_plugin wallet_plugin abi_generator fc ${PLATFORM_SPECIFIC_LIBS} )

target_include_directories( plugin_test PUBLIC ${CMAKE_SOURCE_DIR}/plugins/net_plugin/include )
add_dependencies(plugin_test asserter test_api test_api_mem test_api_db test_api_multi_index exchange proxy identity identity_test stltest infinite eosio.system eosio.token eosio.bios test.inline multi_index_test noop dice eosio.msig)

#
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/p2p_tests/sync/test.sh ${CMAKE_CURRENT_BINARY_DIR}/p2p_tests/sync/test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/p2p_tests/dawn_515/test.sh ${CMAKE_CURRENT_BINARY_DIR}/p2p_tests/dawn_515/test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/trans_sync_across_mixed_cluster_test.sh ${CMAKE_CURRENT_BINARY_DIR}/trans_sync_across_mixed_cluster_test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-remote-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-remote-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sample-cluster-map.json ${CMAKE_CURRENT_BINARY_DIR}/sample-cluster-map.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/restart-scenarios-test.py ${CMAKE_CURRENT_BINARY_DIR}/restart-scenarios-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testUtils.py ${CMAKE_CURRENT_BINARY_DIR}/testUtils.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_run_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_run_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_run_remote_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_run_remote_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/consensus-validation-malicious-producers.py ${CMAKE_CURRENT_BINARY_DIR}/consensus-validation-malicious-producers.py COPYONLY)

#To run plugin_test with all log from blockchain displayed, put --verbose after --, i.e. plugin_test -- --verbose
add_test(NAME plugin_test COMMAND plugin_test --report_level=detailed --color_output)
# TODO removed on slim: add_test(NAME nodeos_run_test COMMAND tests/nodeos_run_test.py -v --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO removed on slim: add_test(NAME nodeos_run_remote_test COMMAND tests/nodeos_run_remote_test.py -v --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO removed on slim: add_test(NAME p2p_dawn515_test COMMAND tests/p2p_tests/dawn_515/test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
#if(BUILD_MONGO_DB_PLUGIN)
#   add_test(NAME nodeos_run_test-mongodb COMMAND tests/nodeos_run_test.py --mongodb -v --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
#endif()

# TODO: Tests removed until working again on master.
# TODO: add_test(NAME p2p_sync_test COMMAND tests/p2p_tests/sync/test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME p2p_sync_test_p2_d10 COMMAND tests/p2p_tests/sync/test.sh -p 2 -d 10 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME message_storm COMMAND tests/p2p_tests/sync/test.sh -m -p 21 -n 21 -d 5 -l WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME trans_sync_across_mixed_cluster_test COMMAND tests/trans_sync_across_mixed_cluster_test.sh -p 1 -n 2 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO removed on slim: add_test(NAME distributed-transactions-test COMMAND tests/distributed-transactions-test.py -p 1 -n 4 -v --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO removed on slim: add_test(NAME distributed-transactions-remote-test COMMAND tests/distributed-transactions-remote-test.py -v --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO removed on slim: add_test(NAME restart-scenarios-test_resync COMMAND tests/restart-scenarios-test.py -c resync -p4 -v --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# add_test(NAME restart-scenarios-test_replay COMMAND tests/restart-scenarios-test.py -c replay -p4 -v --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# TODO: add_test(NAME consensus-validation-malicious-producers COMMAND tests/consensus-validation-malicious-producers.py -w 80 --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

if(ENABLE_COVERAGE_TESTING)

  set(Coverage_NAME ${PROJECT_NAME}_coverage)

  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT LLVMCOV_PATH)
    message(FATAL_ERROR "llvm-cov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif() # NOT GENHTML_PATH

  # no spaces allowed within tests list
  set(ctest_tests 'plugin_test|p2p_dawn515_test|nodeos_run_test|distributed-transactions-test|restart-scenarios-test_resync')
  set(ctest_exclude_tests 'nodeos_run_remote_test|nodeos_run_test-mongodb|distributed-transactions-remote-test|restart-scenarios-test_replay')

  # Setup target
  add_custom_target(${Coverage_NAME}

    COMMAND ${LCOV_PATH} --directory . --zerocounters

    COMMAND ctest -R ${ctest_tests} -E ${ctest_exclude_tests}

    COMMENT "Capturing lcov counters and generating report"
    COMMAND ${LCOV_PATH} --directory . --capture --gcov-tool ./tools/llvm-gcov.sh --output-file ${Coverage_NAME}.info

    COMMENT "Processing code coverage counters and generating report."
    COMMAND ${GENHTML_PATH} -o ${Coverage_NAME} ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info
    COMMAND ${CMAKE_COMMAND} -E remove ${Coverage_NAME}.base ${Coverage_NAME}.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned

    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Resetting code coverage counters to zero. Processing code coverage counters and generating report."
    )

  # Show info where to find the report
  add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
    COMMAND ;
    COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
    )
endif()
