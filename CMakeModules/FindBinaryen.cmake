# - Try to find BINARYEN
# Once done this will define
#  BINARYEN_FOUND - System has BINARYEN
#  BINARYEN_INCLUDE_DIRS - The BINARYEN include directories
#  BINARYEN_LIBRARIES - The libraries needed to use BINARYEN
#  BINARYEN_DEFINITIONS - Compiler switches required for using BINARYEN

find_program(BINARYEN_BIN s2wasm HINTS ${BINARYEN_ROOT}/bin)
find_path(BINARYEN_INCLUDE_DIRS binaryen-c.h HINTS ${BINARYEN_ROOT}/include)
find_library(BINARYEN_LIBRARIES binaryen HINTS ${BINARYEN_ROOT}/lib)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set BINARYEN_FOUND to TRUE
# if all listed variables are TRUE

find_package_handle_standard_args(BINARYEN DEFAULT_MSG BINARYEN_BIN BINARYEN_INCLUDE_DIRS BINARYEN_LIBRARIES )

mark_as_advanced(BINARYEN_BIN BINARYEN_INCLUDE_DIRS BINARYEN_LIBRARIES)
