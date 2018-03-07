# - Try to find SOFTFLOAT

# TODO: Check if compiler is able to generate wasm32
message( STATUS "Looking for softfloat library" )
find_library( SOFTFLOAT_LIB libsoftfloat.a PATHS ${SOFTFLOAT_ROOT} NO_DEFAULT_PATH )
