cmake_minimum_required(VERSION 3.8)

project(example_todo)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)
include(EosioWasmToolchain)

set(EOSIO_TESTER_PATH "eosio-tester")
set(EOSIO_EXECUTABLES_PATH "" CACHE PATH "Path to directory containing EOSIO executables")

set(ROOT_BINARY ${CMAKE_CURRENT_BINARY_DIR})
set(ROOT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})

function(add_contract TARGET SRC)
    add_executable(${TARGET} ${SRC})
    target_compile_options(${TARGET} PUBLIC -no-missing-ricardian-clause)
    target_include_directories(${TARGET} PUBLIC
        ${ROOT_SOURCE}/libraries/
    )
endfunction()

add_contract(todo_contract todo_contract.cpp)
add_contract(todo_filter todo_filter.cpp)

add_subdirectory(libraries)
add_subdirectory(tests)
