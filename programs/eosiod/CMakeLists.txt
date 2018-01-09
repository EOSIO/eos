add_executable( eosiod main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling eosiod with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "eosiod_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${eosiod_BUILD_VERSION}")
  else()
    set(eosiod_BUILD_VERSION 0)
  endif()
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(eosiod PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( eosiod
        PRIVATE appbase
        PRIVATE account_history_api_plugin account_history_plugin
        PRIVATE -Wl,--whole-archive chain_api_plugin -Wl,--no-whole-archive producer_plugin chain_plugin
        PRIVATE -Wl,--whole-archive wallet_api_plugin -Wl,--no-whole-archive
        PRIVATE net_plugin net_api_plugin -Wl,--whole-archive txn_test_gen_plugin -Wl,--no-whole-archive
        PRIVATE http_plugin -Wl,--whole-archive faucet_testnet_plugin -Wl,--no-whole-archive
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   eosiod

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
