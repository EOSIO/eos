enable_testing()
include(ExternalProject)

set( EOSIO_TESTER_EXE ${CMAKE_BINARY_DIR}/bin/eosio-tester )

if( EOSIO_COMPILE_RODEOS_TESTS )
   set(EOSIO_WASM_OLD_BEHAVIOR "Off")
   find_package(eosio.cdt REQUIRED)
   message( STATUS "Building rodeos tests in directory `libraries/rodeos/tests/wasm`" )
   ExternalProject_Add(
      test_rodeos_wasms_project
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm
      BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasm
      CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_TOOLCHAIN_FILE=${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake
        -DEOSIO_COMPILE_RODEOS_TESTS=${EOSIO_COMPILE_RODEOS_TESTS}
        -DEOSIO_TESTER_EXE=${EOSIO_TESTER_EXE}
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      TEST_COMMAND ""
      INSTALL_COMMAND ""
      BUILD_ALWAYS 1
   )
else()
  message( STATUS "Not building rodeos tests in directory `libraries/rodeos/tests/wasm/`" )
  add_subdirectory(wasm)
endif()
