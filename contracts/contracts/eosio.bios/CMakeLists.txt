if (EOSIO_COMPILE_TEST_CONTRACTS)
   add_contract(eosio.bios eosio.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.bios.cpp)

   target_include_directories(eosio.bios
      PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include)

   set_target_properties(eosio.bios
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.bios.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/eosio.bios.contracts.md @ONLY )

   target_compile_options( eosio.bios PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
else()
   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/bin/eosio.bios.abi ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY )
   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/bin/eosio.bios.wasm ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY )
endif()
