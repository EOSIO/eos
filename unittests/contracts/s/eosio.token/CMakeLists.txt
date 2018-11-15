add_contract(eosio.token eosio.token ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.token.cpp)
target_include_directories(eosio.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eosio.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
