set(GMP_USAGES "On;Auto;Off")
set(GMP_USAGE "On" CACHE STRING "libgmp usage: On: required; Auto: used if present; Off: not used")
set_property(CACHE GMP_USAGE PROPERTY STRINGS ${GMP_USAGES})

if(NOT GMP_USAGE IN_LIST GMP_USAGES)
   message(FATAL_ERROR "GMP_USAGE must be set to one of: ${GMP_USAGES}")
endif()

if(GMP_USAGE STREQUAL "On")
   find_package(GMP REQUIRED)
elseif(GMP_USAGE)
   find_package(GMP)
endif()

if(GMP_FOUND)
   set(SECP256K1_CONFIG_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config_with_gmp")
else()
   set(SECP256K1_CONFIG_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config_without_gmp")
   unset(GMP_INCLUDE_DIR CACHE)
   unset(GMP_LIBRARIES CACHE)
endif()

add_library(secp256k1 STATIC
  upstream/src/secp256k1.c
)

target_include_directories(secp256k1
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/upstream/
        ${CMAKE_CURRENT_SOURCE_DIR}/upstream/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src
        ${SECP256K1_CONFIG_INC_DIR}
        ${GMP_INCLUDE_DIR}
)

target_compile_definitions(secp256k1 PRIVATE HAVE_CONFIG_H=1)

target_link_libraries(secp256k1 ${GMP_LIBRARIES})
install( TARGETS secp256k1 
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
