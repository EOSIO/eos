add_contract(rem.msig rem.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.msig.cpp)

target_include_directories(rem.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.msig PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
