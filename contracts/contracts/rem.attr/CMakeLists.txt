add_contract(rem.attr rem.attr ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.attr.cpp)

target_include_directories(rem.attr
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.attr
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.attr PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
