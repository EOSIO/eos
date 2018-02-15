# Tries to find binaryen as a library.
#
# Usage of this module as follows:
#
#     find_package(binaryen)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  BINARYEN_ROOT        Set this variable to the root installation of
#                       binaryen if the module has problems finding
#                       the proper installation path.
#
# Variables defined by this module:
#
#  BINARYEN_FOUND                System has binaryen libs/headers
#  BINARYEN_LIBRARIES            The binaryen libraries
#  BINARYEN_INCLUDE_DIR          The location of binaryen headers

set(BINARYEN_HINTS ${BINARYEN_ROOT}/lib ${BINARYEN_BIN}/../lib)

find_library(BINARYEN_WASM_LIBRARY
   NAMES wasm
   HINTS ${BINARYEN_HINTS})

find_library(BINARYEN_ASMJS_LIBRARY
   NAMES asmjs
   HINTS ${BINARYEN_HINTS})

find_library(BINARYEN_PASSES_LIBRARY
   NAMES passes
   HINTS ${BINARYEN_HINTS})

find_library(BINARYEN_CFG_LIBRARY
   NAMES cfg
   HINTS ${BINARYEN_HINTS})

find_library(BINARYEN_AST_LIBRARY
   NAMES ast
   HINTS ${BINARYEN_HINTS})

find_library(BINARYEN_EMSCRIPTEN_OPTIMIZER_LIBRARY
   NAMES emscripten-optimizer
   HINTS ${BINARYEN_HINTS})

find_library(BINARYEN_SUPPORT_LIBRARY
   NAMES support
   HINTS ${BINARYEN_HINTS})

set(BINARYEN_LIBRARIES 
   ${BINARYEN_WASM_LIBRARY}
   ${BINARYEN_ASMJS_LIBRARY} 
   ${BINARYEN_PASSES_LIBRARY} 
   ${BINARYEN_CFG_LIBRARY} 
   ${BINARYEN_AST_LIBRARY} 
   ${BINARYEN_EMSCRIPTEN_OPTIMIZER_LIBRARY} 
   ${BINARYEN_SUPPORT_LIBRARY} )

find_path(BINARYEN_INCLUDE_DIR
    NAMES wasm.h wasm-binary.h wasm-interpreter.h
    HINTS ${BINARYEN_ROOT}/src ${BINARYEN_BIN}/../src)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    binaryen
    DEFAULT_MSG
    BINARYEN_LIBRARIES
    BINARYEN_INCLUDE_DIR)

mark_as_advanced(
    BINARYEN_LIBRARIES
    BINARYEN_INCLUDE_DIR
    BINARYEN_WASM_LIBRARY
    BINARYEN_ASMJS_LIBRARY
    BINARYEN_PASSES_LIBRARY
    BINARYEN_CFG_LIBRARY
    BINARYEN_AST_LIBRARY
    BINARYEN_EMSCRIPTEN_OPTIMIZER_LIBRARY
    BINARYEN_SUPPORT_LIBRARY)
