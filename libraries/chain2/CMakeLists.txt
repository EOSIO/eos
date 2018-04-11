file(GLOB HEADERS "include/eosio/chain/*.hpp" "include/eosio/chain/contracts/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( eosio_chain2
             merkle.cpp
             name.cpp
             transaction.cpp
             block_header.cpp
             block_header_state.cpp
             block_state.cpp
             fork_database.cpp
             controller.cpp
#             chain_config.cpp
#             block_trace.cpp
#             wast_to_wasm.cpp
#             wasm_interface.cpp
#             wasm_eosio_validation.cpp
#             wasm_eosio_injection.cpp
#             apply_context.cpp
#             resource_limits.cpp

#             fork_database.cpp
#             get_config.cpp
#             block_log.cpp
#             asset.cpp


#             global_property_object.cpp
#             chain_controller.cpp
#
#             contracts/eosio_contract.cpp
#             contracts/chain_initializer.cpp
#             contracts/genesis_state.cpp
#             contracts/abi_serializer.cpp

#             webassembly/wavm.cpp
#             webassembly/binaryen.cpp

#             transaction_metadata.cpp
             ${HEADERS}
             )

target_link_libraries( eosio_chain2 eos_utilities fc chainbase Logging IR WAST WASM Runtime
      wasm asmjs passes cfg ast emscripten-optimizer support softfloat 
                     )
target_include_directories( eosio_chain2
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../../externals/binaryen/src"
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)


