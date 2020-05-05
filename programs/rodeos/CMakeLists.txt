add_executable( ${RODEOS_EXECUTABLE_NAME} 
  cloner_plugin.cpp
  main.cpp
  rocksdb_plugin.cpp
  wasm_ql_http.cpp
  wasm_ql_plugin.cpp
)

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "rodeos_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${rodeos_BUILD_VERSION}")
  else()
    set(rodeos_BUILD_VERSION 0)
  endif()
else()
  set(rodeos_BUILD_VERSION 0)
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(${RODEOS_EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/abieos/src )

target_link_libraries( ${RODEOS_EXECUTABLE_NAME}
        PRIVATE appbase version
        PRIVATE rodeos_lib fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

copy_bin( ${RODEOS_EXECUTABLE_NAME} )
install( TARGETS
   ${RODEOS_EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT base
)
