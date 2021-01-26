find_program( NODE_FOUND node )

include_directories( "${CMAKE_SOURCE_DIR}/plugins/wallet_plugin/include" )

file(GLOB UNIT_TESTS "*.cpp")

add_executable( plugin_test ${UNIT_TESTS} )
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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/block_log_util_test.py ${CMAKE_CURRENT_BINARY_DIR}/block_log_util_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/distributed-transactions-remote-test.py ${CMAKE_CURRENT_BINARY_DIR}/distributed-transactions-remote-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sample-cluster-map.json ${CMAKE_CURRENT_BINARY_DIR}/sample-cluster-map.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/restart-scenarios-test.py ${CMAKE_CURRENT_BINARY_DIR}/restart-scenarios-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/terminate-scenarios-test.py ${CMAKE_CURRENT_BINARY_DIR}/terminate-scenarios-test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_startup_catchup.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_startup_catchup.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_forked_chain_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_forked_chain_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_short_fork_take_over_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_short_fork_take_over_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_run_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_run_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_run_remote_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_run_remote_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_under_min_avail_ram.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_under_min_avail_ram.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_voting_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_voting_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_irreversible_mode_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_irreversible_mode_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_chainbase_allocation_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_chainbase_allocation_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_protocol_feature_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_protocol_feature_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_multiple_version_protocol_feature_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_multiple_version_protocol_feature_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_extra_packed_data_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_extra_packed_data_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/consensus-validation-malicious-producers.py ${CMAKE_CURRENT_BINARY_DIR}/consensus-validation-malicious-producers.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/validate-dirty-db.py ${CMAKE_CURRENT_BINARY_DIR}/validate-dirty-db.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/launcher_test.py ${CMAKE_CURRENT_BINARY_DIR}/launcher_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/keosd_auto_launch_test.py ${CMAKE_CURRENT_BINARY_DIR}/keosd_auto_launch_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/db_modes_test.sh ${CMAKE_CURRENT_BINARY_DIR}/db_modes_test.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/prod_preactivation_test.py ${CMAKE_CURRENT_BINARY_DIR}/prod_preactivation_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/release-build.sh ${CMAKE_CURRENT_BINARY_DIR}/release-build.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/validate-reflection.py ${CMAKE_CURRENT_BINARY_DIR}/validate-reflection.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version-label.sh ${CMAKE_CURRENT_BINARY_DIR}/version-label.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_producer_watermark_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_producer_watermark_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ship_test.py ${CMAKE_CURRENT_BINARY_DIR}/ship_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ship_client.js ${CMAKE_CURRENT_BINARY_DIR}/ship_client.js COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_high_transaction_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_high_transaction_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/light_validation_sync_test.py ${CMAKE_CURRENT_BINARY_DIR}/light_validation_sync_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosio_blocklog_prune_test.py ${CMAKE_CURRENT_BINARY_DIR}/eosio_blocklog_prune_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cli_test.py ${CMAKE_CURRENT_BINARY_DIR}/cli_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugin_http_api_test.py ${CMAKE_CURRENT_BINARY_DIR}/plugin_http_api_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resource_monitor_plugin_test.py ${CMAKE_CURRENT_BINARY_DIR}/resource_monitor_plugin_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_filter.wasm ${CMAKE_CURRENT_BINARY_DIR}/test_filter.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/trace_plugin_test.py ${CMAKE_CURRENT_BINARY_DIR}/trace_plugin_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nodeos_contrl_c_test.py ${CMAKE_CURRENT_BINARY_DIR}/nodeos_contrl_c_test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/blockvault_tests.py ${CMAKE_CURRENT_BINARY_DIR}/blockvault_tests.py COPYONLY)

#To run plugin_test with all log from blockchain displayed, put --verbose after --, i.e. plugin_test -- --verbose
add_test(NAME plugin_test COMMAND plugin_test --report_level=detailed --color_output)

add_test(NAME nodeos_sanity_test COMMAND tests/nodeos_run_test.py -v --sanity-test --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_sanity_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME nodeos_run_test COMMAND tests/nodeos_run_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_run_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME block_log_util_test COMMAND tests/block_log_util_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST block_log_util_test PROPERTY LABELS nonparallelizable_tests)

if(NODE_FOUND)
  add_test(NAME ship_test COMMAND tests/ship_test.py -v --num-clients 1 --num-requests 5000 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TEST ship_test PROPERTY LABELS nonparallelizable_tests)
endif(NODE_FOUND)

add_test(NAME p2p_dawn515_test COMMAND tests/p2p_tests/dawn_515/test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST p2p_dawn515_test PROPERTY LABELS nonparallelizable_tests)

add_test(NAME producer-preactivate-feature-test COMMAND tests/prod_preactivation_test.py --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST producer-preactivate-feature-test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME nodeos_protocol_feature_test COMMAND tests/nodeos_protocol_feature_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_protocol_feature_test PROPERTY LABELS nonparallelizable_tests)

add_test(NAME distributed-transactions-test COMMAND tests/distributed-transactions-test.py -d 2 -p 4 -n 6 -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST distributed-transactions-test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME restart-scenarios-test-resync COMMAND tests/restart-scenarios-test.py -c resync -p4 -v --clean-run --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST restart-scenarios-test-resync PROPERTY LABELS nonparallelizable_tests)
add_test(NAME restart-scenarios-test-hard_replay COMMAND tests/restart-scenarios-test.py -c hardReplay -p4 -v --clean-run --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST restart-scenarios-test-hard_replay PROPERTY LABELS nonparallelizable_tests)
add_test(NAME restart-scenarios-test-none COMMAND tests/restart-scenarios-test.py -c none --kill-sig term -p4 -v --clean-run --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST restart-scenarios-test-none PROPERTY LABELS nonparallelizable_tests)
add_test(NAME terminate-scenarios-test-resync COMMAND tests/terminate-scenarios-test.py -p 1 -c resync --kill-count 1 --terminate-at-block 10 --kill-sig term --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST terminate-scenarios-test-resync PROPERTY LABELS nonparallelizable_tests)
add_test(NAME terminate-scenarios-test-replay COMMAND tests/terminate-scenarios-test.py -p 1 -c replay --kill-count 1 --terminate-at-block 10 --kill-sig term --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST terminate-scenarios-test-replay PROPERTY LABELS nonparallelizable_tests)
add_test(NAME terminate-scenarios-test-hard_replay COMMAND tests/terminate-scenarios-test.py -p 1 -c hardReplay --kill-count 1 --terminate-at-block 10 --kill-sig term --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST terminate-scenarios-test-hard_replay PROPERTY LABELS nonparallelizable_tests)
# TODO: add_test(NAME consensus-validation-malicious-producers COMMAND tests/consensus-validation-malicious-producers.py -w 80 --dump-error-details WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME validate_dirty_db_test COMMAND tests/validate-dirty-db.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST validate_dirty_db_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME launcher_test COMMAND tests/launcher_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST launcher_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME keosd_auto_launch_test COMMAND tests/keosd_auto_launch_test.py WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST keosd_auto_launch_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME db_modes_test COMMAND tests/db_modes_test.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(db_modes_test PROPERTIES COST 6000)
add_test(NAME release-build-test COMMAND tests/release-build.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME validate-reflection-test COMMAND "${CMAKE_BINARY_DIR}/tests/validate-reflection.py" plugins/ programs/ libraries/ --recurse --extension "cpp" --extension "hpp" -e WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME version-label-test COMMAND tests/version-label.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME light_validation_sync_test COMMAND tests/light_validation_sync_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST light_validation_sync_test PROPERTY LABELS nonparallelizable_tests)
add_test(NAME eosio_blocklog_prune_test COMMAND tests/eosio_blocklog_prune_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST eosio_blocklog_prune_test PROPERTY LABELS nonparallelizable_tests)

# Long running tests
add_test(NAME nodeos_sanity_lr_test COMMAND tests/nodeos_run_test.py -v --sanity-test --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_sanity_lr_test PROPERTY LABELS long_running_tests)
add_test(NAME nodeos_run_check_lr_test COMMAND tests/nodeos_run_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_run_check_lr_test PROPERTY LABELS long_running_tests)
add_test(NAME nodeos_remote_lr_test COMMAND tests/nodeos_run_remote_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_remote_lr_test PROPERTY LABELS long_running_tests)

#add_test(NAME distributed_transactions_lr_test COMMAND tests/distributed-transactions-test.py -d 2 -p 21 -n 21 -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
#set_property(TEST distributed_transactions_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_forked_chain_lr_test COMMAND tests/nodeos_forked_chain_test.py -v --wallet-port 9901 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_forked_chain_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_contrl_c_lr_test COMMAND tests/nodeos_contrl_c_test.py -v --wallet-port 9901 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_contrl_c_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_voting_lr_test COMMAND tests/nodeos_voting_test.py -v --wallet-port 9902 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_voting_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_under_min_avail_ram_lr_test COMMAND tests/nodeos_under_min_avail_ram.py -v --wallet-port 9904 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(nodeos_under_min_avail_ram_lr_test PROPERTIES TIMEOUT 3000)
set_property(TEST nodeos_under_min_avail_ram_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_irreversible_mode_lr_test COMMAND tests/nodeos_irreversible_mode_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_irreversible_mode_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_chainbase_allocation_lr_test COMMAND tests/nodeos_chainbase_allocation_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_chainbase_allocation_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_startup_catchup_lr_test COMMAND tests/nodeos_startup_catchup.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(nodeos_startup_catchup_lr_test PROPERTIES TIMEOUT 3000)
set_property(TEST nodeos_startup_catchup_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_short_fork_take_over_lr_test COMMAND tests/nodeos_short_fork_take_over_test.py -v --wallet-port 9905 --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_short_fork_take_over_lr_test PROPERTY LABELS long_running_tests)

if(ENABLE_MULTIVERSION_PROTOCOL_TEST)
  set(ALTERNATE_VERSION_LABELS_FILE "${CMAKE_BINARY_DIR}/tests/multiversion_paths.conf")

  add_test(NAME nodeos_multiple_version_protocol_feature_mv_test COMMAND tests/nodeos_multiple_version_protocol_feature_test.py
           -v --clean-run --dump-error-detail --alternate-version-labels-file ${ALTERNATE_VERSION_LABELS_FILE} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TEST nodeos_multiple_version_protocol_feature_mv_test PROPERTY LABELS mixed_version_tests)

  add_test(NAME nodeos_extra_packed_data_multiversion_test COMMAND tests/nodeos_extra_packed_data_test.py 
         --alternate-version-labels-file ${ALTERNATE_VERSION_LABELS_FILE} -v --clean-run -p 8 --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TEST nodeos_extra_packed_data_multiversion_test PROPERTY LABELS mixed_version_tests)
endif()

add_test(NAME nodeos_extra_packed_data_test COMMAND tests/nodeos_extra_packed_data_test.py -v --clean-run -p 8 --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_extra_packed_data_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_producer_watermark_lr_test COMMAND tests/nodeos_producer_watermark_test.py -v --clean-run --dump-error-detail WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_producer_watermark_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_high_transaction_lr_test COMMAND tests/nodeos_high_transaction_test.py -v --clean-run --dump-error-detail -p 4 -n 8 --num-transactions 10000 --max-transactions-per-second 500 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_high_transaction_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME nodeos_repeat_transaction_lr_test COMMAND tests/nodeos_high_transaction_test.py -v --clean-run --dump-error-detail -p 4 -n 8 --num-transactions 1000 --max-transactions-per-second 500 --send-duplicates WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST nodeos_repeat_transaction_lr_test PROPERTY LABELS long_running_tests)

add_test(NAME cli_test COMMAND tests/cli_test.py WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_test(NAME plugin_http_api_test COMMAND tests/plugin_http_api_test.py WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(plugin_http_api_test PROPERTIES TIMEOUT 40)
set_property(TEST plugin_http_api_test PROPERTY LABELS nonparallelizable_tests)

add_test(NAME trace_plugin_test COMMAND tests/trace_plugin_test.py WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(trace_plugin_test PROPERTIES TIMEOUT 100)
set_property(TEST trace_plugin_test PROPERTY LABELS nonparallelizable_tests)

add_subdirectory(se_tests)

add_test(NAME resource_monitor_plugin_test COMMAND tests/resource_monitor_plugin_test.py WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST resource_monitor_plugin_test PROPERTY LABELS long_running_tests)

if (pqxx_FOUND )
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/scripts/postgres_control.sh status OUTPUT_VARIABLE HAS_POSTGRES_SERVER)
  if (HAS_POSTGRES_SERVER)
    add_test(NAME blockvault_tests COMMAND tests/blockvault_tests.py --dump-error-detail -v --clean-run WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_property(TEST blockvault_tests PROPERTY LABELS nonparallelizable_tests)
  endif()
endif()

if(ENABLE_COVERAGE_TESTING)

  set(Coverage_NAME ${PROJECT_NAME}_coverage)

  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT LLVMCOV_PATH)
    message(FATAL_ERROR "llvm-cov not found! Aborting...")
  endif()

  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif() # NOT GENHTML_PATH

  # no spaces allowed within tests list
  set(ctest_tests 'plugin_test|p2p_dawn515_test|nodeos_run_test|distributed-transactions-test|restart-scenarios-test_resync')
  set(ctest_exclude_tests 'nodeos_run_remote_test|distributed-transactions-remote-test|restart-scenarios-test_replay')

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
