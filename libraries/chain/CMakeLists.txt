configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/apifiny/chain/core_symbol.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/apifiny/chain/core_symbol.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis_state_root_key.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp)

file(GLOB HEADERS "include/apifiny/chain/*.hpp"
                  "include/apifiny/chain/webassembly/*.hpp"
                  "${CMAKE_CURRENT_BINARY_DIR}/include/apifiny/chain/core_symbol.hpp" )

if(APPLE AND UNIX)
   set(PLATFORM_TIMER_IMPL platform_timer_macos.cpp)
else()
   try_run(POSIX_TIMER_TEST_RUN_RESULT POSIX_TIMER_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/platform_timer_posix_test.c)
   if(POSIX_TIMER_TEST_RUN_RESULT EQUAL 0)
      set(PLATFORM_TIMER_IMPL platform_timer_posix.cpp)
   else()
      set(PLATFORM_TIMER_IMPL platform_timer_asio_fallback.cpp)
   endif()
endif()

if("apifiny-vm-oc" IN_LIST APIFINY_WASM_RUNTIMES)
   set(CHAIN_APIFINYVMOC_SOURCES webassembly/apifiny-vm-oc/code_cache.cpp
                             webassembly/apifiny-vm-oc/executor.cpp
                             webassembly/apifiny-vm-oc/memory.cpp
                             webassembly/apifiny-vm-oc/intrinsic.cpp
                             webassembly/apifiny-vm-oc/LLVMJIT.cpp
                             webassembly/apifiny-vm-oc/LLVMEmitIR.cpp
                             webassembly/apifiny-vm-oc/compile_monitor.cpp
                             webassembly/apifiny-vm-oc/compile_trampoline.cpp
                             webassembly/apifiny-vm-oc/ipc_helpers.cpp
                             webassembly/apifiny-vm-oc/gs_seg_helpers.c
                             webassembly/apifiny-vm-oc.cpp)

   if(LLVM_VERSION VERSION_LESS 7.1 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      enable_language(ASM-LLVMWAR)
      list(APPEND CHAIN_APIFINYVMOC_SOURCES webassembly/apifiny-vm-oc/llvmWARshim.llvmwar)
   else()
      list(APPEND CHAIN_APIFINYVMOC_SOURCES webassembly/apifiny-vm-oc/llvmWARshim.cpp)
   endif()

   llvm_map_components_to_libnames(LLVM_LIBS support core passes mcjit native orcjit)
   include_directories(${LLVM_INCLUDE_DIRS})
   add_definitions(${LLVM_DEFINITIONS})

   option(APIFINYVMOC_ENABLE_DEVELOPER_OPTIONS "enable developer options for APIFINY VM OC" OFF)
endif()

if("apifiny-vm" IN_LIST APIFINY_WASM_RUNTIMES OR "apifiny-vm-jit" IN_LIST APIFINY_WASM_RUNTIMES)
   set(CHAIN_APIFINYVM_SOURCES "webassembly/apifiny-vm.cpp")
   set(CHAIN_APIFINYVM_LIBRARIES apifiny-vm)
endif()

## SORT .cpp by most likely to change / break compile
add_library( apifiny_chain
             merkle.cpp
             name.cpp
             transaction.cpp
             block.cpp
             block_header.cpp
             block_header_state.cpp
             block_state.cpp
             fork_database.cpp
             controller.cpp
             authorization_manager.cpp
             resource_limits.cpp
             block_log.cpp
             transaction_context.cpp
             apifiny_contract.cpp
             apifiny_contract_abi.cpp
             apifiny_contract_abi_bin.cpp
             chain_config.cpp
             chain_id_type.cpp
             genesis_state.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp

#             chain_config.cpp
#             block_trace.cpp
              wast_to_wasm.cpp
              wasm_interface.cpp
              wasm_apifiny_validation.cpp
              wasm_apifiny_injection.cpp
              apply_context.cpp
              abi_serializer.cpp
              asset.cpp
              snapshot.cpp

             webassembly/wabt.cpp
             ${CHAIN_APIFINYVMOC_SOURCES}
             ${CHAIN_APIFINYVM_SOURCES}

#             get_config.cpp
#
#             contracts/chain_initializer.cpp

             trace.cpp
             transaction_metadata.cpp
             protocol_state_object.cpp
             protocol_feature_activation.cpp
             protocol_feature_manager.cpp
             producer_schedule.cpp
             genesis_intrinsics.cpp
             whitelisted_intrinsics.cpp
             thread_utils.cpp
             platform_timer_accuracy.cpp
             ${PLATFORM_TIMER_IMPL}
             ${HEADERS}
             )

target_link_libraries( apifiny_chain fc chainbase Logging IR WAST WASM Runtime
                       softfloat builtins wabt ${CHAIN_APIFINYVM_LIBRARIES} ${LLVM_LIBS}
                     )
target_include_directories( apifiny_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/libraries/apifiny-vm/include"
                                   "${CMAKE_SOURCE_DIR}/libraries/wabt"
                                   "${CMAKE_BINARY_DIR}/libraries/wabt"
                            )

if("apifiny-vm-oc" IN_LIST APIFINY_WASM_RUNTIMES)
   target_link_libraries(apifiny_chain "-Wl,-wrap=main")
endif()

foreach(RUNTIME ${APIFINY_WASM_RUNTIMES})
   string(TOUPPER "${RUNTIME}" RUNTIMEUC)
   string(REPLACE "-" "_" RUNTIMEUC ${RUNTIMEUC})
   target_compile_definitions(apifiny_chain PUBLIC "APIFINY_${RUNTIMEUC}_RUNTIME_ENABLED")
endforeach()

if(APIFINYVMOC_ENABLE_DEVELOPER_OPTIONS)
   message(WARNING "APIFINY VM OC Developer Options are enabled; these are NOT supported")
   target_compile_definitions(apifiny_chain PUBLIC APIFINY_APIFINY_VM_OC_DEVELOPER)
endif()

install( TARGETS apifiny_chain
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/apifiny/chain/
      DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/apifiny/chain
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" EXCLUDE
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/apifiny/chain/core_symbol.hpp DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/apifiny/chain)
#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)
