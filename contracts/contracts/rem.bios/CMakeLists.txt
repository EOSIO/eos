add_contract(rem.bios rem.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.bios.cpp)

target_include_directories(rem.bios
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.bios
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/rem.bios.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/rem.bios.contracts.md @ONLY )

target_compile_options( rem.bios PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
