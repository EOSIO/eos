if(NOT "eos-vm-oc" IN_LIST EOSIO_WASM_RUNTIMES)
    return()
endif()

add_executable(eosvmoc_test main.cpp codecache_tests.cpp)

target_link_libraries(eosvmoc_test PRIVATE eosio_chain_wrap eosio_testing eosio_chain)

add_test(NAME eosvmoc_test COMMAND tests/eosvmoc_tests/eosvmoc_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})