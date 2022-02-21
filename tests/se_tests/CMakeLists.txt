if(NOT MAS_ENABLED OR MAS_LEGACY)
    return()
endif()

add_executable(se_test se_tests.cpp)
target_link_libraries(se_test PRIVATE se-helpers fc)

add_test(NAME se_test COMMAND tests/se_tests/se_test.app/Contents/MacOS/se_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

mas_sign(se_test)