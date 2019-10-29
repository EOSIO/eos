add_contract(rem.system rem.system
   ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.system.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/delegate_bandwidth.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/native.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/producer_pay.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/rex.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/voting.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/rotation.cpp
)

target_include_directories(rem.system
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../rem.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../rem.attr/include
)

set_target_properties(rem.system
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_contract(rex.results rex.results ${CMAKE_CURRENT_SOURCE_DIR}/src/rex.results.cpp)

target_include_directories(rex.results
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rex.results
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.rex")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/rem.system.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/rem.system.contracts.md @ONLY )

target_compile_options( rem.system PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
