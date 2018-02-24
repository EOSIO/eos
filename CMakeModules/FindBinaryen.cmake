# - Try to find BINARYEN
# Once done this will define
#  BINARYEN_FOUND - System has BINARYEN

find_program(BINARYEN_BIN s2wasm HINTS ${BINARYEN_ROOT}/bin)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set BINARYEN_FOUND to TRUE
# if all listed variables are TRUE

find_package_handle_standard_args(BINARYEN DEFAULT_MSG BINARYEN_BIN)

mark_as_advanced(BINARYEN_BIN BINARYEN_INCLUDE_DIRS BINARYEN_LIBRARIES)
