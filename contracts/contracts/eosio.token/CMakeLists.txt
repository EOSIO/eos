add_contract(eosio.token eosio.token ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.token.cpp)

target_include_directories(eosio.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eosio.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( eosio.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
