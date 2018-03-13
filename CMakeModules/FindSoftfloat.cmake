# - Try to find SOFTFLOAT

# TODO: Check if compiler is able to generate wasm32
if ("${SOFTFLOAT_ROOT}" STREQUAL "")
   if (NOT "$ENV{OPENSSL_ROOT_DIR}" STREQUAL "")
      set(SOFTFLOAT_ROOT $ENV{SOFTFLOAT_ROOT})
   elseif (APPLE)
      set(SOFTFLOAT_ROOT "/usr/local/berkeley-softfloat-3")
   elseif(UNIX AND NOT APPLE)
      set(SOFTFLOAT_ROOT "/opt/berkeley-softfloat-3")
   else()
      message(FATAL_ERROR "softfloat not found and don't know where to look, please specify SOFTFLOAT_ROOT")
   endif()
endif()
message( STATUS "Looking for softfloat library" )
find_library( SOFTFLOAT_LIB libsoftfloat.a PATHS ${SOFTFLOAT_ROOT} NO_DEFAULT_PATH )
