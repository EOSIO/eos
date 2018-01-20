find_package(Boost 1.64 REQUIRED COMPONENTS unit_test_framework)

set(CMAKE_CXX_STANDARD 14)

add_executable(libeosioclient_test
    test.cpp
    eosioclient_test.cpp
    )
target_include_directories(libeosioclient_test PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_definitions(libeosioclient_test PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(libeosioclient_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} libeosioclient)

add_test(libeosioclient_test libeosioclient_test)
