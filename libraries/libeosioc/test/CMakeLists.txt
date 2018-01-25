find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(libeosioc_test
    test.cpp
    wallet_test.cpp
    peer_test.cpp
    )
target_include_directories(libeosioc_test PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_definitions(libeosioc_test PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(libeosioc_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} libeosioc)

add_test(libeosioc_test libeosioc_test)
