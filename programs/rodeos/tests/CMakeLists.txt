add_executable(test_rodeos_cli test_rodeos_cli.cpp)
target_link_libraries(test_rodeos_cli
    PRIVATE rodeos_lib eosio_chain fc appbase amqpcpp amqp ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
    PRIVATE Boost::unit_test_framework
)

add_test(NAME test_rodeos_cli
    COMMAND programs/rodeos/tests/test_rodeos_cli
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
