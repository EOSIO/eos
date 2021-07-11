
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()

if (CMAKE_TOOLCHAIN_FILE MATCHES ".*vcpkg\\.cmake")
  cmake_minimum_required( VERSION 3.16 )
  option(USE_SYSTEM_OPENSSL "Use system supplied openssl package" OFF)
  set(VCPKG_OVERLAY_PORTS ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-ports)

  if (APPLE)
    option(USE_SYSTEM_LLVM "Use system supplied llvm package" ON)
  else()
    option(USE_SYSTEM_LLVM "Use system supplied llvm package" OFF)
  endif()

  if(USE_SYSTEM_OPENSSL)
    list(APPEND VCPKG_OVERLAY_PORTS ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-openssl-system)
  endif()

  if(NOT USE_SYSTEM_LLVM)
    list(APPEND VCPKG_MANIFEST_FEATURES "vcpkg-llvm")
  endif()

else()
  cmake_minimum_required( VERSION 3.8 )
endif()

project( EOSIO )
include(CTest) # suppresses DartConfiguration.tcl error
enable_testing()

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
   message(WARNING "CMAKE_INSTALL_PREFIX is set to /usr/local. This is not recommended.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

include( GNUInstallDirs )
include( InstallDirectoryPermissions )
include( MASSigning )

set( BLOCKCHAIN_NAME "EOSIO" )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION_SUFFIX rc1)

if(VERSION_SUFFIX)
    set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
    set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

set( CLI_CLIENT_EXECUTABLE_NAME cleos )
set( NODE_EXECUTABLE_NAME nodeos )
set( KEY_STORE_EXECUTABLE_NAME keosd )
set( RODEOS_EXECUTABLE_NAME rodeos )
set( TESTER_EXECUTABLE_NAME eosio-tester )

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        message(FATAL_ERROR "GCC version must be at least 8.0!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        message(FATAL_ERROR "Clang version must be at least 5.0!")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wsign-compare -Wrange-loop-analysis")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" AND NOT _VCPKG_INSTALLED_DIR)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
   add_compile_options(-fdiagnostics-color=always)
endif()

set(BUILD_DOXYGEN FALSE CACHE BOOL "Build doxygen documentation on every make")
set(ENABLE_MULTIVERSION_PROTOCOL_TEST FALSE CACHE BOOL "Enable nodeos multiversion protocol test")

# add defaults for openssl
if(APPLE AND UNIX AND "${OPENSSL_ROOT_DIR}" STREQUAL "")
   set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@1.1")
endif()

# WASM runtimes to enable. Each runtime in this list will have:
#  * definition EOSIO_<RUNTIME>_RUNTIME_ENABLED defined in public libchain interface
#  * ctest entries with --runtime
# TODO reenable later
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
   if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
      list(APPEND EOSIO_WASM_RUNTIMES eos-vm-oc)
      # EOS VM OC requires LLVM, but move the check up here to a central location so that the EosioTester.cmakes
      # can be created with the exact version found
      find_package(LLVM REQUIRED CONFIG)
     if(LLVM_VERSION_MAJOR VERSION_LESS 7 OR LLVM_VERSION_MAJOR VERSION_GREATER_EQUAL 12)
        message(FATAL_ERROR "EOSIO requires an LLVM version 7 through 11")
     endif()
   endif()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
   if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
      list(APPEND EOSIO_WASM_RUNTIMES eos-vm eos-vm-jit)
   endif()
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

set(Boost_USE_MULTITHREADED      ON)
set( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )
# Most boost deps get implictly picked up via fc, as just about everything links to fc. In addition we pick up
# the pthread dependency through fc.
find_package(Boost 1.67 REQUIRED COMPONENTS program_options unit_test_framework)

if (_VCPKG_INSTALLED_DIR)
  find_package(RocksDB CONFIG REQUIRED)
  find_package(fc CONFIG REQUIRED)
  find_package(softfloat CONFIG REQUIRED)
  find_package(appbase CONFIG REQUIRED)
  find_package(chainbase CONFIG REQUIRED)
  find_package(abieos CONFIG REQUIRED)
  find_package(eos-vm CONFIG REQUIRED)
  find_package(yubihsm CONFIG REQUIRED)
  find_package(libpqxx CONFIG REQUIRED)

  add_library(rocksdb ALIAS RocksDB::rocksdb)
  add_library(pqxx ALIAS libpqxx::pqxx)
endif()

if( APPLE AND UNIX )
# Apple Specific Options Here
    message( STATUS "Configuring EOSIO on macOS" )
    set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wno-deprecated-declarations" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-declarations" )
else()
    # Linux Specific Options Here
    message( STATUS "Configuring EOSIO on Linux" )
    set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
    if ( FULL_STATIC_BUILD )
      set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
    endif ( FULL_STATIC_BUILD )

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
        if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
        endif()
    endif()
endif()

option(EOSIO_ENABLE_DEVELOPER_OPTIONS "enable developer options for EOSIO" OFF)

# based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
# uncomment this line to tell GDB about macros (slows compile times)
# set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build EOSIO for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
  SET(CMAKE_C_FLAGS   "--coverage ${CMAKE_C_FLAGS}")
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

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling EOSIO with TCMalloc")
    #if doing this by the book, simply link_libraries( ${GPERFTOOLS_TCMALLOC} ) here. That will
    #give the performance benefits of tcmalloc but since it won't be linked last
    #the heap profiler & checker may not be accurate. This here is rather undocumented behavior
    #to stuff a library toward the end of the link list
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ${GPERFTOOLS_TCMALLOC}")
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ${GPERFTOOLS_TCMALLOC}")
endif()

add_subdirectory( libraries )
add_subdirectory( plugins )
add_subdirectory( programs )
add_subdirectory( scripts )
add_subdirectory( unittests )
add_subdirectory( contracts )
add_subdirectory( tests )
add_subdirectory( tools )

option(DISABLE_WASM_SPEC_TESTS "disable building of wasm spec unit tests" OFF)

if (NOT DISABLE_WASM_SPEC_TESTS)
  if (NOT _VCPKG_INSTALLED_DIR)
    add_subdirectory( eosio-wasm-spec-tests/generated-tests )
  else()
    include(FetchContent)
    FetchContent_Declare(
      eosio-wasm-spec-tests
      GIT_REPOSITORY https://github.com/EOSIO/eosio-wasm-spec-tests.git
      GIT_TAG        22f7f62d5451ee57f14b2c3b9f62e35da50560f1
    )
    FetchContent_MakeAvailable(eosio-wasm-spec-tests)  
  endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio)

install(FILES testnet.template DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/launcher)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/launcher)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testnet.template ${CMAKE_CURRENT_BINARY_DIR}/etc/eosio/launcher/testnet.template COPYONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/contracts/contracts/eosio.bios/eosio.bios.abi DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/contracts)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/contracts/contracts/eosio.bios/eosio.bios.wasm DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/contracts)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/contracts/contracts/eosio.boot/eosio.boot.abi DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/contracts)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/contracts/contracts/eosio.boot/eosio.boot.wasm DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/contracts)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.in ${CMAKE_CURRENT_BINARY_DIR}/version.hpp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.hpp DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio)



if (_VCPKG_INSTALLED_DIR)

  set(CORE_DEPENDECIES_INSTALLED_DIR ${CMAKE_CURRENT_BINARY_DIR}/core_dependecies)

  add_custom_command(
    OUTPUT ${CORE_DEPENDECIES_INSTALLED_DIR}/vcpkg/status
    COMMAND "${Z_VCPKG_EXECUTABLE}" install
                --triplet "${VCPKG_TARGET_TRIPLET}"
                --vcpkg-root "${Z_VCPKG_ROOT_DIR}"
                "--x-wait-for-lock"
                "--x-manifest-root=${VCPKG_MANIFEST_DIR}"
                "--x-install-root=${CORE_DEPENDECIES_INSTALLED_DIR}"
                "${Z_VCPKG_FEATURE_FLAGS}" "--x-no-default-features"
                ${Z_VCPKG_ADDITIONAL_MANIFEST_PARAMS}
                ${VCPKG_INSTALL_OPTIONS}
    DEPENDS ${_VCPKG_INSTALLED_DIR}/vcpkg/status
  )

  add_custom_target(core_dependencies DEPENDS ${CORE_DEPENDECIES_INSTALLED_DIR}/vcpkg/status)
  install(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target core_dependencies)")
  install(DIRECTORY ${CORE_DEPENDECIES_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include 
                    ${CORE_DEPENDECIES_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
                    ${CORE_DEPENDECIES_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share
    DESTINATION ${CMAKE_INSTALL_PREFIX}
  )


  install(TARGETS Logging IR Runtime WASM WAST builtins chain_kv eosio_chain eosio_testing_contracts eosio_testing
          EXPORT eosio)

  export(EXPORT eosio
    FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/eosio/EosioTargets.cmake"
  )
  include(CMakePackageConfigHelpers)

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/eosio/EosioConfigVersion.cmake"
    VERSION "${VERSION_FULL}"
    COMPATIBILITY ExactVersion
  )

  # create config file
  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/eosio/eosio-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eosio
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  ) 

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/eosio/eosio-config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/eosio/EosioConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/eosio )

  configure_file(${CMAKE_SOURCE_DIR}/libraries/wasm-jit/LICENSE
                ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.wavm COPYONLY)

  set(VCPKG_INSTALLED_SHARE ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share)
  file(COPY ${CMAKE_SOURCE_DIR}/LICENSE
            ${VCPKG_INSTALLED_SHARE}/rocksdb/LICENSE.leveldb 
            ${VCPKG_INSTALLED_SHARE}/fc/LICENSE.go
      DESTINATION ${CMAKE_BINARY_DIR}/licenses/eosio
  )

  file(GLOB_RECURSE COPYRIGHT_FILES "${VCPKG_INSTALLED_SHARE}/*/copyright")
  foreach(f ${COPYRIGHT_FILES})
    if(f MATCHES "${VCPKG_INSTALLED_SHARE}/(.*)/copyright")
      set(lib ${CMAKE_MATCH_1})
      if (NOT lib MATCHES "boost-.*")
        configure_file(${VCPKG_INSTALLED_SHARE}/${lib}/copyright ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.${lib} COPYONLY)
      elseif(NOT EXISTS ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.boost)
        configure_file(${VCPKG_INSTALLED_SHARE}/${lib}/copyright ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.boost COPYONLY)
      endif()
    endif()
  endforeach()

  install(DIRECTORY ${CMAKE_BINARY_DIR}/licenses/eosio DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses COMPONENT base)

else()

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
configure_file(${CMAKE_SOURCE_DIR}/libraries/softfloat/COPYING.txt
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.softfloat COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/wasm-jit/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.wavm COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/fc/secp256k1/secp256k1/COPYING
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.secp256k1 COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/fc/include/fc/crypto/webauthn_json/license.txt
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.rapidjson COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/fc/src/network/LICENSE.go
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.go COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/yubihsm/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.yubihsm COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/eos-vm/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.eos-vm COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/rocksdb/LICENSE.Apache
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.rocksdb COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/rocksdb/LICENSE.leveldb
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.leveldb COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/amqp-cpp/LICENSE
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.amqpcpp COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/libraries/libpqxx/COPYING
               ${CMAKE_BINARY_DIR}/licenses/eosio/LICENSE.libpqxx COPYONLY)

install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ COMPONENT base)
install(FILES libraries/softfloat/COPYING.txt DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.softfloat COMPONENT base)
install(FILES libraries/wasm-jit/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.wavm COMPONENT base)
install(FILES libraries/fc/secp256k1/secp256k1/COPYING DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.secp256k1 COMPONENT base)
install(FILES libraries/fc/include/fc/crypto/webauthn_json/license.txt DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.rapidjson COMPONENT base)
install(FILES libraries/fc/src/network/LICENSE.go DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ COMPONENT base)
install(FILES libraries/yubihsm/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.yubihsm COMPONENT base)
install(FILES libraries/eos-vm/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.eos-vm COMPONENT base)
install(FILES libraries/rocksdb/LICENSE.Apache DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.rocksdb COMPONENT base)
install(FILES libraries/rocksdb/LICENSE.leveldb DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.leveldb COMPONENT base)
install(FILES libraries/amqp-cpp/LICENSE DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.amqpcpp COMPONENT base)
install(FILES libraries/libpqxx/COPYING DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/licenses/eosio/ RENAME LICENSE.libpqxx COMPONENT base)
endif()

add_custom_target(base-install
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
  COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=base -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  USES_TERMINAL
)

get_property(_CTEST_CUSTOM_TESTS_IGNORE GLOBAL PROPERTY CTEST_CUSTOM_TESTS_IGNORE)
file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "SET(CTEST_CUSTOM_TESTS_IGNORE ${_CTEST_CUSTOM_TESTS_IGNORE})")

include(package)
include(doxygen)
