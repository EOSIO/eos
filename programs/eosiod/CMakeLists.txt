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

target_link_libraries( eosiod
        PRIVATE appbase
        PRIVATE -Wl,${whole_archive_flag} account_history_api_plugin -Wl,${no_whole_archive_flag} account_history_plugin
        PRIVATE -Wl,${whole_archive_flag} chain_api_plugin -Wl,${no_whole_archive_flag} producer_plugin chain_plugin
        PRIVATE -Wl,${whole_archive_flag} wallet_api_plugin -Wl,${no_whole_archive_flag}
        PRIVATE net_plugin -Wl,${whole_archive_flag} net_api_plugin txn_test_gen_plugin -Wl,${no_whole_archive_flag} 
        PRIVATE http_plugin -Wl,${whole_archive_flag} faucet_testnet_plugin -Wl,${no_whole_archive_flag}
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

if(BUILD_MONGO_DB_PLUGIN)
  target_link_libraries( eosiod
    PRIVATE -Wl,${whole_archive_flag} mongo_db_plugin -Wl,${no_whole_archive_flag} )
endif()

install( TARGETS
   eosiod

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
