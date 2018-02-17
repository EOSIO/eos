# - Try to find EOS
# Once done this will define
#  EOS_FOUND - System has EOS
#  EOS_INCLUDE_DIRS - The EOS include directories
#  EOS_LIBRARIES - The libraries needed to use EOS
#  EOS_DEFINITIONS - Compiler switches required for using EOS

#/home/asini/opt/wasm/bin/llvm-config

find_file(WASM_LLVM_CONFIG llvm-config HINTS ${WASM_ROOT}/bin)
find_program(WASM_CLANG clang HINTS ${WASM_ROOT}/bin)
find_program(WASM_LLC llc HINTS ${WASM_ROOT}/bin)
find_program(WASM_LLVM_LINK llvm-link HINTS ${WASM_ROOT}/bin)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EOS_FOUND to TRUE
# if all listed variables are TRUE

find_package_handle_standard_args(WASM DEFAULT_MSG WASM_LLVM_CONFIG WASM_CLANG WASM_LLC WASM_LLVM_LINK)

