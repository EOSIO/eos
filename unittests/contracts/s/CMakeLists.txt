add_contract(eosio.system eosio.system ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.system.cpp)
#add_executable(eosio.system.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.system.cpp)
target_include_directories(eosio.system.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include)

set_target_properties(eosio.system.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
