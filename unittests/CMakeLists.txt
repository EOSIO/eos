#file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

set( CMAKE_CXX_STANDARD 14 )

include_directories("${CMAKE_BINARY_DIR}/contracts")
include_directories("${CMAKE_SOURCE_DIR}/contracts")
include_directories("${CMAKE_SOURCE_DIR}/unittests/contracts")
include_directories("${CMAKE_SOURCE_DIR}/libraries/testing/include")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp ESCAPE_QUOTES)

file(GLOB UNIT_TESTS "*.cpp")

add_executable( unit_test ${UNIT_TESTS} ${WASM_UNIT_TESTS} )
target_link_libraries( unit_test eosio_chain chainbase eosio_testing eos_utilities abi_generator fc ${PLATFORM_SPECIFIC_LIBS} )

target_include_directories( unit_test PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/tests/contracts )
target_include_directories( unit_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wasm_tests )
target_include_directories( unit_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )
add_dependencies(unit_test asserter test_api test_api_mem test_api_db test_api_multi_index exchange eosio.token proxy identity identity_test stltest infinite eosio.system eosio.token eosio.bios test.inline multi_index_test noop dice eosio.msig)

#Manually run unit_test for all supported runtimes
#To run unit_test with all log from blockchain displayed, put --verbose after --, i.e. unit_test -- --verbose
add_test(NAME unit_test_binaryen COMMAND unit_test -t \!eosio_system_tests/* -t \!delay_tests/* -t \!currency_tests/test_deferred_failure --report_level=detailed --color_output -- --binaryen)
add_test(NAME unit_test_wavm COMMAND unit_test -t \!eosio_system_tests/* -t \!delay_tests/* -t \!currency_tests/test_deferred_failure --report_level=detailed --color_output --catch_system_errors=no -- --wavm)

if(ENABLE_COVERAGE_TESTING)

  set(Coverage_NAME ${PROJECT_NAME}_ut_coverage)

  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT LLVMCOV_PATH)
    message(FATAL_ERROR "llvm-cov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif() # NOT GENHTML_PATH

  # no spaces allowed within tests list
  set(ctest_tests 'unit_test_binaryen|unit_test_wavm')
  set(ctest_exclude_tests '')

  # Setup target
  add_custom_target(${Coverage_NAME}

    # Cleanup lcov
    COMMAND ${LCOV_PATH} --directory . --zerocounters

    # Run tests
    COMMAND ./tools/ctestwrapper.sh -R ${ctest_tests} -E ${ctest_exclude_tests}

    COMMAND ${LCOV_PATH} --directory . --capture --gcov-tool ./tools/llvm-gcov.sh --output-file ${Coverage_NAME}.info

    COMMAND ${GENHTML_PATH} -o ${Coverage_NAME} ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info
    COMMAND ${CMAKE_COMMAND} -E remove ${Coverage_NAME}.base ${Coverage_NAME}.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned

    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Resetting code coverage counters to zero. Processing code coverage counters and generating report."
    )

  # Show info where to find the report
  add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
    COMMAND ;
    COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
    )
endif()
