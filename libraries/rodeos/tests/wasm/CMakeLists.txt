find_package(eosio.cdt)
include(CTest)
enable_testing()

function(add_tester NAME SRC)
    add_executable(${NAME}.wasm ${SRC})
    target_link_libraries(${NAME}.wasm -ftester -stack-size=65536)
    target_compile_options(${NAME}.wasm PUBLIC -ftester -Os -no-missing-ricardian-clause)
    add_test(NAME ${NAME}.wasm COMMAND eosio-tester ${NAME}.wasm WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

add_contract(legacydb legacydb legacydb-test-contract.cpp)
add_tester(legacydb-tester legacydb-tester.cpp)
