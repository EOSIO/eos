# Defines EOS library target.
project( EOS )
cmake_minimum_required( VERSION 2.8.12 )

set(VERSION_MAJOR 2)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/fc/CMakeModules")

INCLUDE( VersionMacros )
INCLUDE( SetupTargetMacros )

set( BLOCKCHAIN_NAME "Eos" )
set( CMAKE_CXX_STANDARD 14 )

set( CLI_CLIENT_EXECUTABLE_NAME eos_client )
set( GUI_CLIENT_EXECUTABLE_NAME eos )
set( CUSTOM_URL_SCHEME "gcs" )
set( INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347" )

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must be at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version must be at least 3.3!")
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(BUILD_DOXYGEN TRUE CACHE BOOL "Build doxygen documentation on every make")
set(EOS_EGENESIS_JSON "${CMAKE_CURRENT_SOURCE_DIR}/genesis.json" )

#set (ENABLE_INSTALLER 1)
#set (USE_PCH 1)

if (USE_PCH)
  include (cotire)
endif(USE_PCH)

SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )
IF( WIN32 )
  SET(BOOST_ROOT $ENV{BOOST_ROOT})
  set(Boost_USE_MULTITHREADED ON)
  set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
ENDIF(WIN32)
FIND_PACKAGE(Boost 1.64 REQUIRED COMPONENTS
    thread
    date_time
    system
    filesystem
    program_options
    signals
    serialization
    chrono
    unit_test_framework
    context
    locale)

if( WIN32 )

    message( STATUS "Configuring Eos on WIN32")
    set( DB_VERSION 60 )
    set( BDB_STATIC_LIBS 1 )

    set( ZLIB_LIBRARIES "" )
    SET( DEFAULT_EXECUTABLE_INSTALL_DIR bin/ )

    set(CRYPTO_LIB)

    #looks like this flag can have different default on some machines.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

    # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

    # On windows tcl should be installed to the directory pointed by setenv.bat script
    SET(TCL_INCLUDE_PATH $ENV{TCL_ROOT}/include)
    MESSAGE(STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}")

    FIND_PACKAGE(TCL)
    MESSAGE(STATUS "tcl_library: ${TCL_LIBRARY}")

    SET(TCL_LIBS "optimized;${TCL_LIBRARY};debug;")
    get_filename_component(TCL_LIB_PATH "${TCL_LIBRARY}" PATH)
    get_filename_component(TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE)
    get_filename_component(TCL_LIB_EXT "${TCL_LIBRARY}" EXT)

    SET(TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}")
    SET(TCL_LIBRARY ${TCL_LIBS})

else( WIN32 ) # Apple AND Linux

    find_library(READLINE_LIBRARIES NAMES readline)
    find_path(READLINE_INCLUDE_DIR readline/readline.h)
    #if(NOT READLINE_INCLUDE_DIR OR NOT READLINE_LIBRARIES)
    #  MESSAGE(FATAL_ERROR "Could not find lib readline.")
    #endif()

    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring Eos on OS X" )
#set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall -Wno-invalid-partial-specialization -Wno-deprecated-declarations" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall -Wno-deprecated-declarations" )
    else( APPLE )
        # Linux Specific Options Here
        message( STATUS "Configuring Eos on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall" )
        set( rt_library rt )
        set( pthread_library pthread)
        if ( NOT DEFINED crypto_library )
          # I'm not sure why this is here, I guess someone has openssl and can't detect it with find_package()?
          # if you have a normal install, you can define crypto_library to the empty string to avoid a build error
          set( crypto_library crypto)
        endif ()
        if ( FULL_STATIC_BUILD )
          set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif ( FULL_STATIC_BUILD )

        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
                set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
            endif()
        endif()
    endif( APPLE )

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
    endif()

    if( "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    # based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
    # set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif( WIN32 )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build Eos for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

include(wasm)
if(WASM_TOOLCHAIN)
  message(STATUS "Using WASM clang: " ${WASM_CLANG})
  message(STATUS "Using WASM llc: " ${WASM_LLC})
  message(STATUS "Using WASM llvm-link: " ${WASM_LLVM_LINK})
  add_subdirectory(contracts)
else()
  message(STATUS "--------------------------------------------------------------------")
  message(STATUS "No WASM compiler could be found ... (skipping building of contracts)")
  message(STATUS "--------------------------------------------------------------------")
endif()

include(installer)

add_subdirectory( libraries )
add_subdirectory( programs )
add_subdirectory( plugins )
add_subdirectory( scripts )
add_subdirectory( tests )
add_subdirectory( tools )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis.json ${CMAKE_CURRENT_BINARY_DIR}/genesis.json COPYONLY)


include(doxygen)


