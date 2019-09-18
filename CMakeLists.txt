cmake_minimum_required( VERSION 3.8 )

project( EOSIO )
include(CTest) # suppresses DartConfiguration.tcl error
enable_testing()

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
   message(WARNING "CMAKE_INSTALL_PREFIX is set to /usr/local. This is not recommended.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

if (UNIX AND APPLE)
   list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/llvm@7" "/usr/local/opt/gettext")
endif()

include( GNUInstallDirs )
include( InstallDirectoryPermissions )
include( MASSigning )

set( BLOCKCHAIN_NAME "EOSIO" )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 9)
set(VERSION_PATCH 0)
set(VERSION_SUFFIX develop)

if(VERSION_SUFFIX)
    set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
    set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

set( CLI_CLIENT_EXECUTABLE_NAME cleos )
set( NODE_EXECUTABLE_NAME nodeos )
set( KEY_STORE_EXECUTABLE_NAME keosd )

# Conan (https://conan.io/)
include(${CMAKE_SOURCE_DIR}/.conan/conan.cmake)

conan_check(VERSION 1.0.0 REQUIRED)

conan_add_remote(NAME bincrafters INDEX 1
    URL https://api.bintray.com/conan/bincrafters/public-conan)

message("$ENV{PINNED}")

if("$ENV{PINNED}" STREQUAL "false" OR "$ENV{UNPINNED}" STREQUAL "true")
    conan_cmake_run(CONANFILE .conan/conanfile-unpinned.txt
        BASIC_SETUP
        CMAKE_TARGETS
        TARGETS
        NO_OUTPUT_DIRS
        KEEP_RPATHS
        BUILD missing)
else()
    conan_cmake_run(CONANFILE .conan/conanfile.txt
        BASIC_SETUP
        CMAKE_TARGETS
        TARGETS
        NO_OUTPUT_DIRS
        KEEP_RPATHS
        BUILD missing)
endif()

if(UNIX)
  if(APPLE)
    set(Intl_LIBRARY ${CONAN_LIB_DIRS_GETTEXT}/libgnuintl.dylib)
    set(CMAKE_INSTALL_RPATH 
      "@executable_path/../lib"
      "@executable_path"
      "${CMAKE_INSTALL_PREFIX}/lib"
    )
  else()
    set(Intl_LIBRARY ${CONAN_LIB_DIRS_GETTEXT}/libgnuintl.so)
    set(CMAKE_INSTALL_RPATH
      "$ORIGIN/../lib"
      "@executable_path/../lib"
      "@executable_path"
      "${CMAKE_INSTALL_PREFIX}/lib"
    )
  endif()
endif()
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
include_directories(${CONAN_INCLUDE_DIRS_LIBCURL} ${CONAN_INCLUDE_DIRS_OPENSSL})
link_directories(${CONAN_LIB_DIRS_OPENSSL} ${CONAN_LIB_DIRS_LIBCURL} ${CONAN_LIB_DIRS_ZLIB})

execute_process(
    COMMAND mkdir -p ${CONAN_LIB_DIRS_OPENSSL}/pkgconfig
    COMMAND cp ${CMAKE_SOURCE_DIR}/build/OpenSSL.pc ${CONAN_LIB_DIRS_OPENSSL}/pkgconfig/libcrypto.pc
    COMMAND cp ${CMAKE_SOURCE_DIR}/build/OpenSSL.pc ${CONAN_LIB_DIRS_OPENSSL}/pkgconfig/libssl.pc
)

# End Conan

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "GCC version must be at least 7.0!")
    endif()
    if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
       add_compile_options(-fdiagnostics-color=always)
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        message(FATAL_ERROR "Clang version must be at least 5.0!")
    endif()
    if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
       add_compile_options(-fcolor-diagnostics)
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(BUILD_DOXYGEN FALSE CACHE BOOL "Build doxygen documentation on every make")
set(BUILD_MONGO_DB_PLUGIN FALSE CACHE BOOL "Build mongo database plugin")

# add defaults for openssl
if(APPLE AND UNIX AND "${OPENSSL_ROOT_DIR}" STREQUAL "")
   set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()

if(UNIX)
  if(APPLE)
    set(whole_archive_flag "-force_load")
    set(no_whole_archive_flag "")
  else()
    set(whole_archive_flag "--whole-archive")
    set(no_whole_archive_flag "--no-whole-archive")
  endif()
else()
  set(whole_archive_flag "--whole-archive")
  set(no_whole_archive_flag "--no-whole-archive")
endif()

set( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )
# Most boost deps get implictly picked up via fc, as just about everything links to fc. In addition we pick up
# the pthread dependency through fc.
find_package(Boost 1.67 REQUIRED COMPONENTS program_options unit_test_framework)

# WAVM requires LLVM, but move the check up here to a central location so that the EosioTester.cmakes
# can be created with the exact version found
find_package(LLVM REQUIRED CONFIG)
if(LLVM_VERSION_MAJOR VERSION_LESS 7 OR LLVM_VERSION_MAJOR VERSION_GREATER 9)
	message(FATAL_ERROR "EOSIO requires an LLVM version 7.0 to 9.0")
endif()

if( APPLE AND UNIX )
# Apple Specific Options Here
    message( STATUS "Configuring EOSIO on macOS" )
    set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-declarations" )
else()
    # Linux Specific Options Here
    message( STATUS "Configuring EOSIO on Linux" )
    set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -Wall" )
    if ( FULL_STATIC_BUILD )
      set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
    endif ( FULL_STATIC_BUILD )

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
        if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
        endif()
    endif()
endif()

# based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
# uncomment this line to tell GDB about macros (slows compile times)
# set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build EOSIO for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
  SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
  find_program( LCOV_PATH lcov )
  find_program( LLVMCOV_PATH  llvm-cov )
  find_program( GENHTML_PATH NAMES genhtml)
endif()

include(utils)

if ("${CORE_SYMBOL_NAME}" STREQUAL "")
  set( CORE_SYMBOL_NAME "SYS" )
endif()
string(TOUPPER ${CORE_SYMBOL_NAME} CORE_SYMBOL_NAME)

string(LENGTH ${CORE_SYMBOL_NAME} CORE_SYMBOL_NAME_LENGTH)
if (CORE_SYMBOL_NAME_LENGTH GREATER 7)
  message(FATAL_ERROR "CORE_SYMBOL_NAME length must be between 1 and 7 characters")
endif()

message( STATUS "Using '${CORE_SYMBOL_NAME}' as CORE symbol name" )

if ("${EOSIO_ROOT_KEY}" STREQUAL "")
   set(EOSIO_ROOT_KEY "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV")
endif()

message( STATUS "Using '${EOSIO_ROOT_KEY}' as public key for 'eosio' account" )

add_subdirectory( libraries )
add_subdirectory( plugins )
add_subdirectory( programs )
add_subdirectory( scripts )
add_subdirectory( unittests )
add_subdirectory( tests )
add_subdirectory( tools )

install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio)

install(FILES testnet.template DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/launcher)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/launcher)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testnet.template ${CMAKE_CURRENT_BINARY_DIR}/etc/eosio/launcher/testnet.template COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eosio.version.in ${CMAKE_CURRENT_BINARY_DIR}/eosio.version.hpp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eosio.version.hpp DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})

set(EOS_ROOT_DIR ${CMAKE_BINARY_DIR})
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/eosio-config.cmake.in ${CMAKE_BINARY_DIR}/lib/cmake/eosio/eosio-config.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/EosioTesterBuild.cmake.in ${CMAKE_BINARY_DIR}/lib/cmake/eosio/EosioTester.cmake @ONLY)

set(EOS_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/eosio-config.cmake.in ${CMAKE_BINARY_DIR}/modules/eosio-config.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/modules/eosio-config.cmake DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/eosio)
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/EosioTester.cmake.in ${CMAKE_BINARY_DIR}/modules/EosioTester.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/modules/EosioTester.cmake DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/eosio)

configure_file(${CMAKE_SOURCE_DIR}/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/wabt/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.wabt COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/softfloat/COPYING.txt
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.softfloat COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/wasm-jit/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.wavm COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/fc/secp256k1/upstream/COPYING
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.secp256k1 COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/fc/src/network/LICENSE.go
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.go COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/yubihsm/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.yubihsm COPYONLY)

install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ COMPONENT base)
install(FILES libraries/wabt/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.wabt COMPONENT base)
install(FILES libraries/softfloat/COPYING.txt DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.softfloat COMPONENT base)
install(FILES libraries/wasm-jit/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.wavm COMPONENT base)
install(FILES libraries/fc/secp256k1/upstream/COPYING DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.secp256k1 COMPONENT base)
install(FILES libraries/fc/src/network/LICENSE.go DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ COMPONENT base)
install(FILES libraries/yubihsm/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.yubihsm COMPONENT base)

add_custom_target(base-install
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
  COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=base -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  USES_TERMINAL
)

get_property(_CTEST_CUSTOM_TESTS_IGNORE GLOBAL PROPERTY CTEST_CUSTOM_TESTS_IGNORE)
file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "SET(CTEST_CUSTOM_TESTS_IGNORE ${_CTEST_CUSTOM_TESTS_IGNORE})")

include(package)
include(doxygen)
