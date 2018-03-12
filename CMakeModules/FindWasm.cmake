# - Try to find WASM

# TODO: Check if compiler is able to generate wasm32
if ("${WASM_ROOT}" STREQUAL "")
   #   if (NOT "$ENV{WASM_ROOT}" STREQUAL "")
   #   set( WASM_ROOT $ENV{WASM_ROOT} )
   if (APPLE)
      set( WASM_ROOT "/usr/local/wasm" )
   elseif (UNIX AND NOT APPLE)
      set( WASM_ROOT "/opt/wasm" )
   else()
      message(FATAL_ERROR "WASM not found and don't know where to look, please specify WASM_ROOT")
   endif()
endif()
find_program(WASM_CLANG clang PATHS ${WASM_ROOT}/bin NO_DEFAULT_PATH)
find_program(WASM_LLC llc PATHS ${WASM_ROOT}/bin NO_DEFAULT_PATH)
find_program(WASM_LLVM_LINK llvm-link PATHS ${WASM_ROOT}/bin NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EOS_FOUND to TRUE
# if all listed variables are TRUE

find_package_handle_standard_args(WASM REQUIRED_VARS WASM_CLANG WASM_LLC WASM_LLVM_LINK)

