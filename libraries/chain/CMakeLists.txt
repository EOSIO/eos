file(GLOB HEADERS "include/eos/chain/*.hpp" "include/eos/chain/contrats/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( eos_chain
             chain_config.cpp
             name.cpp
             transaction.cpp
             block.cpp
             wast_to_wasm.cpp
             wasm_interface.cpp

             fork_database.cpp
             get_config.cpp
             block_log.cpp


             chain_controller.cpp

             contracts/eos_contract.cpp
             contracts/producer_objects.cpp
             contracts/staked_balance_objects.cpp
             contracts/native_contract_chain_initializer.cpp
             contracts/native_contract_chain_administrator.cpp
             contracts/genesis_state.cpp


             ${HEADERS}
        )

target_link_libraries( eos_chain eos_utilities fc chainbase Logging IR WAST WASM Runtime )
target_include_directories( eos_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                            )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   eos_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/eos/chain" )
