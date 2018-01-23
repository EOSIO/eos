find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable( test_cypher_suites test_cypher_suites.cpp )
target_link_libraries( test_cypher_suites ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} fc )
