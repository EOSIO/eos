add_executable( eosd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling eosd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "eosd_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${eosd_BUILD_VERSION}")
  else()
    set(eosd_BUILD_VERSION 0)
  endif()
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(eosd PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( eosd
        PRIVATE appbase
        PRIVATE account_history_api_plugin net_api_plugin account_history_plugin db_plugin
        PRIVATE chain_api_plugin producer_plugin chain_plugin wallet_api_plugin
        PRIVATE net_plugin http_plugin txn_test_gen_plugin faucet_testnet_plugin
        PRIVATE eos_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   eosd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
