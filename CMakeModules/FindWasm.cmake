# - Try to find WASM

# TODO: Check if compiler is able to generate wasm32

find_program(WASM_CLANG clang PATHS ${WASM_ROOT}/bin NO_DEFAULT_PATH)
find_program(WASM_LLC llc PATHS ${WASM_ROOT}/bin NO_DEFAULT_PATH)
find_program(WASM_LLVM_LINK llvm-link PATHS ${WASM_ROOT}/bin NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EOS_FOUND to TRUE
# if all listed variables are TRUE

find_package_handle_standard_args(WASM REQUIRED_VARS WASM_CLANG WASM_LLC WASM_LLVM_LINK)

