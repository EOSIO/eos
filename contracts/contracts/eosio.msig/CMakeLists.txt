add_contract(eosio.msig eosio.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.msig.cpp)

target_include_directories(eosio.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eosio.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( eosio.msig PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
