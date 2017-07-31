# - Try to find secp256k1 include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(Secp256k1)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Secp256k1_ROOT_DIR         Set this variable to the root installation of
#                            secp256k1 if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  SECP256K1_FOUND            System has secp256k1, include and lib dirs found
#  Secp256k1_INCLUDE_DIR      The secp256k1 include directories.
#  Secp256k1_LIBRARY          The secp256k1 library.

find_path(Secp256k1_ROOT_DIR
    NAMES include/secp256k1.h
)

find_path(Secp256k1_INCLUDE_DIR
    NAMES secp256k1.h
    HINTS ${Secp256k1_ROOT_DIR}/include
)

find_library(Secp256k1_LIBRARY
    NAMES libsecp256k1.a secp256k1.lib secp256k1
    HINTS ${Secp256k1_ROOT_DIR}/lib
)

if(Secp256k1_INCLUDE_DIR AND Secp256k1_LIBRARY)
  set(SECP256K1_FOUND TRUE)
else(Secp256k1_INCLUDE_DIR AND Secp256k1_LIBRARY)
  FIND_LIBRARY(Secp256k1_LIBRARY NAMES secp256k1)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Secp256k1 DEFAULT_MSG Secp256k1_INCLUDE_DIR Secp256k1_LIBRARY )
  MARK_AS_ADVANCED(Secp256k1_INCLUDE_DIR Secp256k1_LIBRARY)
endif(Secp256k1_INCLUDE_DIR AND Secp256k1_LIBRARY)

mark_as_advanced(
    Secp256k1_ROOT_DIR
    Secp256k1_INCLUDE_DIR
    Secp256k1_LIBRARY
)

MESSAGE( STATUS "Found Secp256k1: ${Secp256k1_LIBRARY}" )
