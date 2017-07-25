# - Try to find secp256k1 include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(SECP256K1)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  SECP256K1_ROOT_DIR         Set this variable to the root installation of
#                            secp256k1 if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  SECP256K1_FOUND            System has secp256k1, include and lib dirs found
#  SECP256K1_INCLUDE_DIR      The secp256k1 include directories.
#  SECP256K1_LIBRARY          The secp256k1 library.

find_path(SECP256K1_ROOT_DIR
    NAMES include/secp256k1.h
)

find_path(SECP256K1_INCLUDE_DIR
    NAMES secp256k1.h
    HINTS ${SECP256K1_ROOT_DIR}/include
)

find_library(SECP256K1_LIBRARY
    NAMES secp256k1
    HINTS ${SECP256K1_ROOT_DIR}/lib
)

if(SECP256K1_INCLUDE_DIR AND SECP256K1_LIBRARY)
  set(SECP256K1_FOUND TRUE)
else(SECP256K1_INCLUDE_DIR AND SECP256K1_LIBRARY)
  FIND_LIBRARY(SECP256K1_LIBRARY NAMES secp256k1)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(SECP256K1 DEFAULT_MSG SECP256K1_INCLUDE_DIR SECP256K1_LIBRARY )
  MARK_AS_ADVANCED(SECP256K1_INCLUDE_DIR SECP256K1_LIBRARY)
endif(SECP256K1_INCLUDE_DIR AND SECP256K1_LIBRARY)

mark_as_advanced(
    SECP256K1_ROOT_DIR
    SECP256K1_INCLUDE_DIR
    SECP256K1_LIBRARY
)

MESSAGE( STATUS "Found SECP256K1: ${SECP256K1_LIBRARY}" )
