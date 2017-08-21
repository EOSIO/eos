add_subdirectory( fc )
add_subdirectory( chainbase )
add_subdirectory( wasm-jit )

if(WASM_TOOLCHAIN)
  add_subdirectory( binaryen )
endif()

add_subdirectory( types )
add_subdirectory( chain )
add_subdirectory( egenesis )
add_subdirectory( utilities )
add_subdirectory( appbase )
add_subdirectory( native_contract )
