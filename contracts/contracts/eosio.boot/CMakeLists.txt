if (EOSIO_COMPILE_TEST_CONTRACTS)
   add_contract(eosio.boot eosio.boot ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.boot.cpp)

   target_include_directories(eosio.boot
      PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include)

   set_target_properties(eosio.boot
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.boot.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/eosio.boot.contracts.md @ONLY )

   target_compile_options( eosio.boot PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
else()
   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/bin/eosio.boot.abi ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY )
   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/bin/eosio.boot.wasm ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY )
endif()

