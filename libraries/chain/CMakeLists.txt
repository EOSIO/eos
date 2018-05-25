configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/enumivo/chain/core_symbol.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/enumivo/chain/core_symbol.hpp)

# set the chain id
if (NOT "${CHAIN_ID}" STREQUAL "")
   set(enumivo_CHAIN_ID "${CHAIN_ID}")
else()
   ## sha256("enumivo::chain version 1.0")
   set(enumivo_CHAIN_ID "144B6AB7924EA47BAE7A5846B4494A35563AE816E2ED5AFFA7A9C990C2B3CFB3")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/chain_id.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/chain_id.cpp)

file(GLOB HEADERS "include/enumivo/chain/*.hpp" "include/enumivo/chain/contracts/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( enumivo_chain
             merkle.cpp
             name.cpp
             transaction.cpp
             block_header.cpp
             block_header_state.cpp
             block_state.cpp
             fork_database.cpp
             controller.cpp
             authorization_manager.cpp
             resource_limits.cpp
             block_log.cpp
             genesis_state.cpp
             transaction_context.cpp
             enumivo_contract.cpp
             enumivo_contract_abi.cpp
             chain_config.cpp
             chain_id.cpp

#             chain_config.cpp
#             block_trace.cpp
              wast_to_wasm.cpp
              wasm_interface.cpp
              wasm_enumivo_validation.cpp
              wasm_enumivo_injection.cpp
              apply_context.cpp
              abi_serializer.cpp
              asset.cpp

             webassembly/wavm.cpp
             webassembly/binaryen.cpp

#             get_config.cpp
#             global_property_object.cpp
#
#             contracts/chain_initializer.cpp


#             transaction_metadata.cpp
             ${HEADERS}
             )

target_link_libraries( enumivo_chain enu_utilities fc chainbase Logging IR WAST WASM Runtime
      wasm asmjs passes cfg ast emscripten-optimizer support softfloat builtins
                     )
target_include_directories( enumivo_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../../externals/binaryen/src"
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)
