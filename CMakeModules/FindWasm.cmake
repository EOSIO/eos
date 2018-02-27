# - Try to find WASM
# Once done this will define
#  WASM_FOUND - System has WASM

# TODO: Check if compiler is able to generate wasm32

find_file(WASM_LLVM_CONFIG llvm-config HINTS ${WASM_ROOT}/bin)
find_program(WASM_CLANG clang HINTS ${WASM_ROOT}/bin)
find_program(WASM_LLC llc HINTS ${WASM_ROOT}/bin)
find_program(WASM_LLVM_LINK llvm-link HINTS ${WASM_ROOT}/bin)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EOS_FOUND to TRUE
# if all listed variables are TRUE

find_package_handle_standard_args(WASM DEFAULT_MSG WASM_LLVM_CONFIG WASM_CLANG WASM_LLC WASM_LLVM_LINK)

