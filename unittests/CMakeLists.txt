find_package( Gperftools QUIET )

### Build contracts with cdt if available ###
include(ExternalProject)

if( EOSIO_COMPILE_TEST_CONTRACTS )
   set(EOSIO_WASM_OLD_BEHAVIOR "Off")
   find_package(eosio.cdt REQUIRED)

   message( STATUS "Building contracts in directory `eos/unittests/test-contracts/`" )
   ExternalProject_Add(
      test_contracts_project
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test-contracts
      BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/test-contracts
      CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake -DEOSIO_COMPILE_TEST_CONTRACTS=${EOSIO_COMPILE_TEST_CONTRACTS}
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      TEST_COMMAND ""
      INSTALL_COMMAND ""
      BUILD_ALWAYS 1
   )
else()
  message( STATUS "Not building contracts in directory `eos/unittests/test-contracts/`" )
  add_subdirectory(test-contracts)
endif()

if( GPERFTOOLS_FOUND )
   message( STATUS "Found gperftools; compiling tests with TCMalloc" )
   list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

add_subdirectory(contracts)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/contracts.hpp ESCAPE_QUOTES)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz1.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz1.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz2.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz2.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz3.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz3.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz4.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz4.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz5.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz5.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz6.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz6.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz7.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz7.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz8.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz8.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz9.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz9.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz10.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz10.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz11.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz11.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz12.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz12.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz13.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz13.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz14.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz14.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/fuzz15.wasm ${CMAKE_CURRENT_BINARY_DIR}/fuzz15.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/big_allocation.wasm ${CMAKE_CURRENT_BINARY_DIR}/big_allocation.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/crash_section_size_too_big.wasm ${CMAKE_CURRENT_BINARY_DIR}/crash_section_size_too_big.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_no_destructor.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_no_destructor.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_readExports.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_readExports.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_readFunctions.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_readFunctions.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_readFunctions_2.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_readFunctions_2.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_readFunctions_3.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_readFunctions_3.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_readGlobals.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_readGlobals.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_readImports.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_readImports.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/leak_wasm_binary_cpp_L1249.wasm ${CMAKE_CURRENT_BINARY_DIR}/leak_wasm_binary_cpp_L1249.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/readFunctions_slowness_out_of_memory.wasm ${CMAKE_CURRENT_BINARY_DIR}/readFunctions_slowness_out_of_memory.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/locals-yc.wasm ${CMAKE_CURRENT_BINARY_DIR}/locals-yc.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/locals-s.wasm ${CMAKE_CURRENT_BINARY_DIR}/locals-s.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/slowwasm_localsets.wasm ${CMAKE_CURRENT_BINARY_DIR}/slowwasm_localsets.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/getcode_deepindent.wasm ${CMAKE_CURRENT_BINARY_DIR}/getcode_deepindent.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/indent-mismatch.wasm ${CMAKE_CURRENT_BINARY_DIR}/indent-mismatch.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/deep_loops_ext_report.wasm ${CMAKE_CURRENT_BINARY_DIR}/deep_loops_ext_report.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/80k_deep_loop_with_ret.wasm ${CMAKE_CURRENT_BINARY_DIR}/80k_deep_loop_with_ret.wasm COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contracts/80k_deep_loop_with_void.wasm ${CMAKE_CURRENT_BINARY_DIR}/80k_deep_loop_with_void.wasm COPYONLY)

### BUILD UNIT TEST EXECUTABLE ###
file(GLOB UNIT_TESTS "*.cpp") # find all unit test suites
add_executable( unit_test ${UNIT_TESTS}) # build unit tests as one executable

target_link_libraries( unit_test eosio_chain chainbase eosio_testing fc appbase ${PLATFORM_SPECIFIC_LIBS} )

target_compile_options(unit_test PUBLIC -DDISABLE_EOSLIB_SERIALIZE)
target_include_directories( unit_test PUBLIC
                            ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            ${CMAKE_SOURCE_DIR}/test-contracts
                            ${CMAKE_BINARY_DIR}/contracts
                            ${CMAKE_CURRENT_SOURCE_DIR}/contracts
                            ${CMAKE_CURRENT_BINARY_DIR}/contracts
                            ${CMAKE_CURRENT_BINARY_DIR}/include )

### MARK TEST SUITES FOR EXECUTION ###
foreach(TEST_SUITE ${UNIT_TESTS}) # create an independent target for each test suite
  execute_process(COMMAND bash -c "grep -E 'BOOST_AUTO_TEST_SUITE\\s*[(]' ${TEST_SUITE} | grep -vE '//.*BOOST_AUTO_TEST_SUITE\\s*[(]' | cut -d ')' -f 1 | cut -d '(' -f 2" OUTPUT_VARIABLE SUITE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE) # get the test suite name from the *.cpp file
  if (NOT "" STREQUAL "${SUITE_NAME}") # ignore empty lines
    execute_process(COMMAND bash -c "echo ${SUITE_NAME} | sed -e 's/s$//' | sed -e 's/_test$//'" OUTPUT_VARIABLE TRIMMED_SUITE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE) # trim "_test" or "_tests" from the end of ${SUITE_NAME}
    # to run unit_test with all log from blockchain displayed, put "--verbose" after "--", i.e. "unit_test -- --verbose"
    add_test(NAME ${TRIMMED_SUITE_NAME}_unit_test_wavm COMMAND unit_test --run_test=${SUITE_NAME} --report_level=detailed --color_output --catch_system_errors=no -- --wavm)
    add_test(NAME ${TRIMMED_SUITE_NAME}_unit_test_wabt COMMAND unit_test --run_test=${SUITE_NAME} --report_level=detailed --color_output -- --wabt)
    # build list of tests to run during coverage testing
    if(NOT "" STREQUAL "${ctest_tests}")
      set(ctest_tests "${ctest_tests}|${TRIMMED_SUITE_NAME}_unit_test_wavm|${TRIMMED_SUITE_NAME}_unit_test_wabt")
    else()
      set(ctest_tests "${TRIMMED_SUITE_NAME}_unit_test_wavm|${TRIMMED_SUITE_NAME}_unit_test_wabt")
    endif()
  endif()
endforeach(TEST_SUITE)
set(ctest_tests "'${ctest_tests}' -j8") # surround test list string in apostrophies

### COVERAGE TESTING ###
if(ENABLE_COVERAGE_TESTING)
  set(Coverage_NAME ${PROJECT_NAME}_ut_coverage)
  # check for dependencies
  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif()
  if(NOT LLVMCOV_PATH)
    message(FATAL_ERROR "llvm-cov not found! Aborting...")
  endif()
  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif()
  # tests to skip during coverage testing
  set(ctest_exclude_tests '') # no spaces allowed within tests list
  # setup target
  add_custom_target(${Coverage_NAME}
    # cleanup lcov
    COMMAND ${LCOV_PATH} --directory . --zerocounters
    # run tests
    COMMAND ./tools/ctestwrapper.sh -R ${ctest_tests} -E ${ctest_exclude_tests}
    COMMAND ${LCOV_PATH} --directory . --capture --gcov-tool ${CMAKE_SOURCE_DIR}/tools/llvm-gcov.sh --output-file ${Coverage_NAME}.info
    COMMAND ${LCOV_PATH} -remove ${Coverage_NAME}.info '*/boost/*' '/usr/lib/*' '/usr/include/*' '*/externals/*' '*/fc/*' '*/wasm-jit/*' --output-file ${Coverage_NAME}_filtered.info
    COMMAND ${GENHTML_PATH} -o ${Coverage_NAME} ${PROJECT_BINARY_DIR}/${Coverage_NAME}_filtered.info
    COMMAND if [ "$CI" != "true" ]\; then ${CMAKE_COMMAND} -E remove ${Coverage_NAME}.base ${Coverage_NAME}.info ${Coverage_NAME}_filtered.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned ${PROJECT_BINARY_DIR}/${Coverage_NAME}_filtered.info.cleaned\; fi
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Resetting code coverage counters to zero. Processing code coverage counters and generating report. Report published in ./${Coverage_NAME}"
  )
  # show info where to find the report
  add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
    COMMAND ;
    COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
  )
endif()
