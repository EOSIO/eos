
set( HEADERS
    ${CMAKE_SOURCE_DIR}/libraries/fc/include
    ${CMAKE_SOURCE_DIR}/externals/magic_get/include
    ${CMAKE_SOURCE_DIR}/libraries/wasm-jit/Include
    ${CMAKE_SOURCE_DIR}/contracts
    ${CMAKE_SOURCE_DIR}/libraries/chainbase/include
    ${CMAKE_SOURCE_DIR}/libraries/chain/include
    ${CMAKE_SOURCE_DIR}/libraries/wabt
    ${CMAKE_BINARY_DIR}/libraries/wabt
    ${CMAKE_SOURCE_DIR}/libraries/builtins
    ${CMAKE_SOURCE_DIR}/externals/binaryen/src 
    ${CMAKE_SOURCE_DIR}/libraries/vm/vm_interface
    ${Boost_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    )

set(SRCS vm_wasm.cpp
    ${CMAKE_SOURCE_DIR}/libraries/chain/wasm_interface.cpp
    ${CMAKE_SOURCE_DIR}/libraries/chain/wasm_eosio_injection.cpp
    ${CMAKE_SOURCE_DIR}/libraries/chain/wasm_eosio_validation.cpp
    ${CMAKE_SOURCE_DIR}/libraries/chain/webassembly/wabt.cpp
    ${CMAKE_SOURCE_DIR}/libraries/chain/webassembly/wavm.cpp
    )

if (APPLE)
    set(_symbols_list "${CMAKE_SOURCE_DIR}/libraries/vm/vm_wasm/symbols.list")
    set(LINK_FLAGS "${LINK_FLAGS} -Wl,-exported_symbols_list,'${_symbols_list}'")
else()
    set(_version_script "${CMAKE_SOURCE_DIR}/libraries/vm/vm_wasm/version.script")
    set(LINK_FLAGS "${LINK_FLAGS} -Wl,--version-script,\"${_version_script}\"")
endif (APPLE)

set(WASM_DEPEND_LIBS Logging IR WAST WASM Runtime Emscripten softfloat builtins eosiolib_native chain_api fc)

add_library( vm_wasm-1 SHARED ${SRCS} )
target_link_libraries( vm_wasm-1 wabt ${WASM_DEPEND_LIBS} )
target_include_directories( vm_wasm-1
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            PUBLIC ${HEADERS}
                            )
target_compile_options(vm_wasm-1     PRIVATE -D_INDEX=1)
set_target_properties(vm_wasm-1  PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

add_library( vm_wasm-2 SHARED ${SRCS} )
target_link_libraries( vm_wasm-2 wabt ${WASM_DEPEND_LIBS} )
target_include_directories( vm_wasm-2
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            PUBLIC ${HEADERS}
                            )
target_compile_options(vm_wasm-2 PRIVATE   -D_INDEX=2)
set_target_properties(vm_wasm-2  PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

add_library( vm_wasm-3 SHARED ${SRCS} )
target_link_libraries( vm_wasm-3 wabt ${WASM_DEPEND_LIBS} )
target_include_directories( vm_wasm-3
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            PUBLIC ${HEADERS}
                            )
target_compile_options(vm_wasm-3 PRIVATE -D_INDEX=3)
set_target_properties(vm_wasm-3  PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

add_library( vm_wasm-4 SHARED ${SRCS} )
target_link_libraries( vm_wasm-4 wabt ${WASM_DEPEND_LIBS} )
target_include_directories( vm_wasm-4
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            PUBLIC ${HEADERS}
                            )
target_compile_options(vm_wasm-4 PRIVATE -D_INDEX=4)
set_target_properties(vm_wasm-4  PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

