cmake_minimum_required(VERSION 3.5)

if( EOSIO_COMPILE_TEST_CONTRACTS )
   include(ExternalProject)
   
   find_package(eosio.cdt)
   
   message(STATUS "Building bios and boot contracts")
   
   set(EOSIO_CDT_VERSION_MIN "1.7")
   set(EOSIO_CDT_VERSION_SOFT_MAX "1.7")
   
   ### Check the version of eosio.cdt
   set(VERSION_MATCH_ERROR_MSG "")
   EOSIO_CHECK_VERSION(VERSION_OUTPUT "${EOSIO_CDT_VERSION}"
                                      "${EOSIO_CDT_VERSION_MIN}"
                                      "${EOSIO_CDT_VERSION_SOFT_MAX}"
                                      "${EOSIO_CDT_VERSION_HARD_MAX}"
                                      VERSION_MATCH_ERROR_MSG)
   if(VERSION_OUTPUT STREQUAL "MATCH")
      message(STATUS "Using eosio.cdt version ${EOSIO_CDT_VERSION}")
   elseif(VERSION_OUTPUT STREQUAL "WARN")
      message(WARNING "Using eosio.cdt version ${EOSIO_CDT_VERSION} even though it exceeds the maximum supported version of ${EOSIO_CDT_VERSION_SOFT_MAX}; continuing with configuration, however build may fail.\nIt is recommended to use eosio.cdt version ${EOSIO_CDT_VERSION_SOFT_MAX}.x")
   else() # INVALID OR MISMATCH
      message(FATAL_ERROR "Found eosio.cdt version ${EOSIO_CDT_VERSION} but it does not satisfy version requirements: ${VERSION_MATCH_ERROR_MSG}\nPlease use eosio.cdt version ${EOSIO_CDT_VERSION_SOFT_MAX}.x")
   endif(VERSION_OUTPUT STREQUAL "MATCH")
   
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(TEST_BUILD_TYPE "Debug")
      set(CMAKE_BUILD_TYPE "Release")
   else()
      set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
   endif()
   
   if (APPLE)
      set(OPENSSL_ROOT "/usr/local/opt/openssl")
   elseif (UNIX)
      set(OPENSSL_ROOT "/usr/include/openssl")
   endif()
   set(SECP256K1_ROOT "/usr/local")
   
   if (APPLE)
      set(OPENSSL_ROOT "/usr/local/opt/openssl")
   elseif (UNIX)
      set(OPENSSL_ROOT "/usr/include/openssl")
   endif()
   set(SECP256K1_ROOT "/usr/local")
   
   string(REPLACE ";" "|" TEST_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
   string(REPLACE ";" "|" TEST_FRAMEWORK_PATH "${CMAKE_FRAMEWORK_PATH}")
   string(REPLACE ";" "|" TEST_MODULE_PATH "${CMAKE_MODULE_PATH}")
   
   set(EOSIO_WASM_OLD_BEHAVIOR "Off")
   # Remove visibility set by top CMakeList
   set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)
   # Remove C++ compiler set by top CMakeList (contracts use eosio-cpp)
   unset(CMAKE_CXX_STANDARD)
   
   find_package(eosio.cdt)
endif()

add_subdirectory(eosio.bios)
add_subdirectory(eosio.boot)
