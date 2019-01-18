
find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

set( CMAKE_CXX_STANDARD 14 )

include_directories("${CMAKE_SOURCE_DIR}/plugins/wallet_plugin/include")

file(GLOB UNIT_TESTS "*.cpp")

add_executable( plugin_test ${UNIT_TESTS} ${WASM_UNIT_TESTS} )
target_link_libraries( plugin_test eosio_testing eosio_chain chainbase chain_plugin wallet_plugin fc ${PLATFORM_SPECIFIC_LIBS} )

target_include_directories( plugin_test PUBLIC
                            ${CMAKE_SOURCE_DIR}/plugins/net_plugin/include
                            ${CMAKE_SOURCE_DIR}/plugins/chain_plugin/include
                            ${CMAKE_BINARY_DIR}/unittests/include/ )
                            
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/core_symbol.py.in ${CMAKE_CURRENT_BINARY_DIR}/core_symbol.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testUtils.py ${CMAKE_CURRENT_BINARY_DIR}/testUtils.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/WalletMgr.py ${CMAKE_CURRENT_BINARY_DIR}/WalletMgr.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Node.py ${CMAKE_CURRENT_BINARY_DIR}/Node.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Cluster.py ${CMAKE_CURRENT_BINARY_DIR}/Cluster.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestHelper.py ${CMAKE_CURRENT_BINARY_DIR}/TestHelper.py COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/p2p_tests/dawn_515/test.sh ${CMAKE_CURRENT_BINARY_DIR}/p2p_tests/dawn_515/test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-remote-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-remote-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sample-cluster-map.json ${CMAKE_CURRENT_BINARY_DIR}/sample-cluster-map.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/restart-scenarios-test.py ${CMAKE_CURRENT_BINARY_DIR}/restart-scenarios-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_forked_chain_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_forked_chain_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_run_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_run_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_run_remote_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_run_remote_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_under_min_avail_ram.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_under_min_avail_ram.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_voting_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_voting_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/consensus-validation-malicious-producers.py ${CMAKE_CURRENT_BINARY_DIR}/consensus-validation-malicious-producers.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/validate-dirty-db.py ${CMAKE_CURRENT_BINARY_DIR}/validate-dirty-db.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/launcher_test.py ${CMAKE_CURRENT_BINARY_DIR}/launcher_test.py COPYONLY)

#To run plugin_test with all log from blockchain displayed, put --verbose after --, i.e. plugin_test -- --verbose
add_test(NAME plugin_test COMMAND plugin_test --report_level=detailed --color_output)

add_test(NAME nodeos_sanity_test COMMAND tests/nodeos_run_test.py -v --sanity-test --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_sanity_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME nodeos_sanity_bnet_test COMMAND tests/nodeos_run_test.py -v --sanity-test --clean-run --p2p-plugin bnet --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_sanity_bnet_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME nodeos_run_test COMMAND tests/nodeos_run_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_run_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME nodeos_run_bnet_test COMMAND tests/nodeos_run_test.py -v --clean-run --p2p-plugin bnet --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_run_bnet_test PROPERTY LABELS nonparallelizable_tests)

add_test(NAME p2p_dawn515_test COMMAND tests/p2p_tests/dawn_515/test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST p2p_dawn515_test PROPERTY LABELS nonparallelizable_tests)
if(BUILD_MONGO_DB_PLUGIN)
  add_test(NAME nodeos_run_test-mongodb COMMAND tests/nodeos_run_test.py --mongodb -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TEST nodeos_run_test-mongodb PROPERTY LABELS nonparallelizable_tests)
endif()

add_test(NAME distributed-transactions-test COMMAND tests/distributed-transactions-test.py -d 2 -p 4 -n 6 -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST distributed-transactions-test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME distributed-transactions-bnet-test COMMAND tests/distributed-transactions-test.py -d 2 -p 1 -n 4 --p2p-plugin bnet -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST distributed-transactions-bnet-test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME restart-scenarios-test-resync COMMAND tests/restart-scenarios-test.py -c resync -p4 -v --clean-run --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST restart-scenarios-test-resync PROPERTY LABELS nonparallelizable_tests)
add_test(NAME restart-scenarios-test-hard_replay COMMAND tests/restart-scenarios-test.py -c hardReplay -p4 -v --clean-run --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST restart-scenarios-test-hard_replay PROPERTY LABELS nonparallelizable_tests)
add_test(NAME restart-scenarios-test-none COMMAND tests/restart-scenarios-test.py -c none --kill-sig term -p4 -v --clean-run --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST restart-scenarios-test-none PROPERTY LABELS nonparallelizable_tests)
# TODO: add_test(NAME consensus-validation-malicious-producers COMMAND tests/consensus-validation-malicious-producers.py -w 80 --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME validate_dirty_db_test COMMAND tests/validate-dirty-db.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST validate_dirty_db_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME launcher_test COMMAND tests/launcher_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST launcher_test PROPERTY LABELS nonparallelizable_tests)

# Long running tests
add_test(NAME nodeos_sanity_lr_test COMMAND tests/nodeos_run_test.py -v --sanity-test --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_sanity_lr_test PROPERTY LABELS long_running_tests)
add_test(NAME nodeos_sanity_bnet_lr_test COMMAND tests/nodeos_run_test.py -v --sanity-test --p2p-plugin bnet --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_sanity_bnet_lr_test PROPERTY LABELS long_running_tests)
add_test(NAME nodeos_run_check_lr_test COMMAND tests/nodeos_run_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_run_check_lr_test PROPERTY LABELS long_running_tests)
add_test(NAME nodeos_remote_lr_test COMMAND tests/nodeos_run_remote_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_remote_lr_test PROPERTY LABELS long_running_tests)

#add_test(NAME distributed_transactions_lr_test COMMAND tests/distributed-transactions-test.py -d 2 -p 21 -n 21 -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
#set_property(TEST distributed_transactions_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_forked_chain_lr_test COMMAND tests/nodeos_forked_chain_test.py -v --wallet-port 9901 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_forked_chain_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_voting_lr_test COMMAND tests/nodeos_voting_test.py -v --wallet-port 9902 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_voting_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_voting_bnet_lr_test COMMAND tests/nodeos_voting_test.py -v --wallet-port 9903 --p2p-plugin bnet --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_voting_bnet_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_under_min_avail_ram_lr_test COMMAND tests/nodeos_under_min_avail_ram.py -v --wallet-port 9904 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_under_min_avail_ram_lr_test PROPERTY LABELS long_running_tests)


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
  set(ctest_tests 'plugin_test|p2p_dawn515_test|nodeos_run_test|bnet_nodeos_run_test|distributed-transactions-test|restart-scenarios-test_resync')
  set(ctest_exclude_tests 'nodeos_run_remote_test|nodeos_run_test-mongodb|distributed-transactions-remote-test|restart-scenarios-test_replay')

  # Setup target
  add_custom_target(${Coverage_NAME}

    COMMAND ${LCOV_PATH} --directory . --zerocounters

    COMMAND ctest -R ${ctest_tests} -E ${ctest_exclude_tests}

    COMMENT "Capturing lcov counters and generating report"
    COMMAND ${LCOV_PATH} --directory . --capture --gcov-tool ${CMAKE_SOURCE_DIR}/tools/llvm-gcov.sh --output-file ${Coverage_NAME}.info

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
