add_executable( test_cypher_suites test_cypher_suites.cpp )
target_link_libraries( test_cypher_suites fc )

add_executable( test_webauthn test_webauthn.cpp )
target_link_libraries( test_webauthn fc ${Boost_LIBRARIES})

add_test(NAME test_cypher_suites COMMAND libraries/fc/test/crypto/test_cypher_suites WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_webauthn COMMAND libraries/fc/test/crypto/test_webauthn WORKING_DIRECTORY ${CMAKE_BINARY_DIR})