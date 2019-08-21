find_package( Gperftools QUIET )

### Build contracts with cdt if available ###
include(ExternalProject)

if( GPERFTOOLS_FOUND )
   message( STATUS "Found gperftools; compiling tests with TCMalloc" )
   list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

add_subdirectory(wasms)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wasm_spec_tests.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/wasm_spec_tests.hpp ESCAPE_QUOTES)

### BUILD UNIT TEST EXECUTABLE ###
file(GLOB WASM_TESTS "*.cpp") # find all unit test suites
add_executable( wasm_spec_test ${WASM_TESTS}) # build unit tests as one executable

target_link_libraries( wasm_spec_test eosio_chain chainbase eosio_testing fc appbase ${PLATFORM_SPECIFIC_LIBS} )

target_compile_options(wasm_spec_test PUBLIC -DDISABLE_EOSLIB_SERIALIZE)
target_include_directories( wasm_spec_test PUBLIC
                            ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            ${CMAKE_CURRENT_BINARY_DIR}/include )

### MARK TEST SUITES FOR EXECUTION ###
foreach(TEST_SUITE ${WASM_TESTS}) # create an independent target for each test suite
  execute_process(COMMAND bash -c "grep -E 'BOOST_AUTO_TEST_SUITE\\s*[(]' ${TEST_SUITE} | grep -vE '//.*BOOST_AUTO_TEST_SUITE\\s*[(]' | cut -d ')' -f 1 | cut -d '(' -f 2" OUTPUT_VARIABLE SUITE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE) # get the test suite name from the *.cpp file
  if (NOT "" STREQUAL "${SUITE_NAME}") # ignore empty lines
    execute_process(COMMAND bash -c "echo ${SUITE_NAME} | sed -e 's/s$//' | sed -e 's/_test$//'" OUTPUT_VARIABLE TRIMMED_SUITE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE) # trim "_test" or "_tests" from the end of ${SUITE_NAME}
    # to run unit_test with all log from blockchain displayed, put "--verbose" after "--", i.e. "unit_test -- --verbose"
    add_test(NAME ${TRIMMED_SUITE_NAME}_unit_test_wavm COMMAND wasm_spec_test --run_test=${SUITE_NAME} --report_level=detailed --color_output --catch_system_errors=no -- --wavm)
    add_test(NAME ${TRIMMED_SUITE_NAME}_unit_test_wabt COMMAND wasm_spec_test --run_test=${SUITE_NAME} --report_level=detailed --color_output -- --wabt)
    # build list of tests to run during coverage testing
    if(NOT "" STREQUAL "${ctest_tests}")
      set(ctest_tests "${ctest_tests}|${TRIMMED_SUITE_NAME}_unit_test_wavm|${TRIMMED_SUITE_NAME}_unit_test_wabt")
    else()
      set(ctest_tests "${TRIMMED_SUITE_NAME}_unit_test_wavm|${TRIMMED_SUITE_NAME}_unit_test_wabt")
    endif()
  endif()
endforeach(TEST_SUITE)
set(ctest_tests "'${ctest_tests}' -j8") # surround test list string in apostrophies
