if( EOSIO_COMPILE_TEST_CONTRACTS AND 0 )
   # This contract should not be rebuilt, because the saved snapshots in
   # unittests/snapshots/ contain a copy of the current version of the contract
   # and snapshot_tests requires it to be identical.
   add_contract( snapshot_test snapshot_test snapshot_test.cpp )
else()
   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/snapshot_test.wasm ${CMAKE_CURRENT_BINARY_DIR}/snapshot_test.wasm COPYONLY )
   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/snapshot_test.abi  ${CMAKE_CURRENT_BINARY_DIR}/snapshot_test.abi  COPYONLY )
endif()
