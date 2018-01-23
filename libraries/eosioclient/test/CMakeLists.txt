find_package(Boost REQUIRED COMPONENTS unit_test_framework)

set(CMAKE_CXX_STANDARD 14)

add_executable(eosioclient_test
    test.cpp
    eosioclient_test.cpp
    )
target_include_directories(eosioclient_test PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_definitions(eosioclient_test PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(eosioclient_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} eosioclient)

add_test(eosioclient_test eosioclient_test)
