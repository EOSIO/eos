add_contract(rem.wrap rem.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.wrap.cpp)

target_include_directories(rem.wrap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.wrap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.wrap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
