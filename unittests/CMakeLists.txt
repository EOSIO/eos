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
include_directories("${CMAKE_SOURCE_DIR}/libraries/testing/include")
include_directories("${CMAKE_SOURCE_DIR}/plugins/chain_plugin/include")
include_directories("${CMAKE_SOURCE_DIR}/libraries/appbase/include")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp ESCAPE_QUOTES)

file(GLOB UNIT_TESTS "*.cpp")

add_executable( unit_test ${UNIT_TESTS} ${WASM_UNIT_TESTS} )
target_link_libraries( unit_test eosio_chain chainbase eosio_testing eos_utilities abi_generator fc ${PLATFORM_SPECIFIC_LIBS} )

target_include_directories( unit_test PUBLIC ${CMAKE_BINARY_DIR}/contracts ${CMAKE_CURRENT_BINARY_DIR}/tests/contracts )
target_include_directories( unit_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wasm_tests )
target_include_directories( unit_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )
add_dependencies(unit_test asserter test_api test_api_mem test_api_db test_api_multi_index exchange currency proxy identity identity_test stltest infinite eosio.system eosio.token eosio.bios test.inline multi_index_test noop dice eosio.msig)

#Manually run chain_test for all supported runtimes
#To run chain_test with all log from blockchain displayed, put --verbose after --, i.e. chain_test -- --verbose
add_test(NAME unit_test_binaryen COMMAND chain_test --report_level=detailed --color_output -- --binaryen)
add_test(NAME unit_test_wavm COMMAND chain_test --report_level=detailed --color_output -- --wavm)

if(ENABLE_COVERAGE_TESTING)

  set(Coverage_NAME ${PROJECT_NAME}_coverage)
  set(Coverage_EXECUTABLE unit_test)

  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif() # NOT GENHTML_PATH

  # Setup target
  add_custom_target(${Coverage_NAME}

    # Cleanup lcov
    COMMAND ${LCOV_PATH} --directory . --zerocounters
    # Create baseline to make sure untouched files show up in the report
    COMMAND ${LCOV_PATH} -c -i -d . -o ${Coverage_NAME}.base

    # Run tests
    COMMAND ${Coverage_EXECUTABLE}

    # Capturing lcov counters and generating report
    COMMAND ${LCOV_PATH} --directory . --capture --output-file ${Coverage_NAME}.info
    # add baseline counters
    COMMAND ${LCOV_PATH} -a ${Coverage_NAME}.base -a ${Coverage_NAME}.info --output-file ${Coverage_NAME}.total
    COMMAND ${LCOV_PATH} --remove ${Coverage_NAME}.total ${COVERAGE_EXCLUDES} --output-file ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
    COMMAND ${GENHTML_PATH} -o ${Coverage_NAME} ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
    COMMAND ${CMAKE_COMMAND} -E remove ${Coverage_NAME}.base ${Coverage_NAME}.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned

    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
    )

  # Show info where to find the report
  add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
    COMMAND ;
    COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
    )
endif()
