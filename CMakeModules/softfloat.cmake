find_package(Softfloat)

if ( ${SOFTFLOAT_LIB} STREQUAL "SOFTFLOAT_LIB-NOTFOUND" )
   message( FATAL_ERROR "Unable to find softfloat library" )
   return()
else()
   message( STATUS "Found softfloat library " ${SOFTFLOAT_LIB} )
   set( SOFTFLOAT_INC, ${SOFTFLOAT_ROOT}/include )
   add_library(softfloat_lib STATIC IMPORTED )
   set_target_properties( softfloat_lib PROPERTIES
      IMPORTED_LOCATION "${SOFTFLOAT_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${SOFTFLOAT_INC}"
   )
endif()
