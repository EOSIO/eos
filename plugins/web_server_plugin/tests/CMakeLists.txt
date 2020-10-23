find_package(OpenSSL REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/test_config.hpp ESCAPE_QUOTES)
message(STATUS "generating certificate and private key for https web server unit tests")
execute_process(COMMAND sh -c "openssl req -x509 -nodes -newkey ec -pkeyopt ec_paramgen_curve:secp521r1 \
                              -keyout ${CMAKE_CURRENT_BINARY_DIR}/test_ecdsa.pem -out ${CMAKE_CURRENT_BINARY_DIR}/test_ecdsa.crt\
                              -days 3650 -subj \"/C=US/ST=VA/L=Blacksburg/O=Block.One LLC\"")

add_executable( test_server test_server.cpp )
target_link_libraries( test_server web_server_plugin )
target_include_directories( test_server PUBLIC 
                            "${CMAKE_CURRENT_BINARY_DIR}/include" )


add_test(NAME test_server COMMAND plugins/web_server_plugin/tests/test_server WORKING_DIRECTORY ${CMAKE_BINARY_DIR})