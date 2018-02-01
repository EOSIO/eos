find_package(Boost 1.64 REQUIRED COMPONENTS unit_test_framework)

set(CMAKE_CXX_STANDARD 14)

add_executable(chain_unit_test
    test.cpp
    block_test.cpp
    name_test.cpp
    transaction_test.cpp)
target_include_directories(chain_unit_test PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_definitions(chain_unit_test PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(chain_unit_test eosio_chain)

add_test(chain_unit_test chain_unit_test)
