add_executable( ${BLOCKVAULTD_EXECUTABLE_NAME} main.cpp )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "blockvaultd_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${blockvaultd_BUILD_VERSION}")
  else()
    set(blockvaultd_BUILD_VERSION 0)
  endif()
else()
  set(blockvaultd_BUILD_VERSION 0)
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(${BLOCKVAULTD_EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( ${BLOCKVAULTD_EXECUTABLE_NAME}
        PRIVATE appbase version fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS})

copy_bin( ${BLOCKVAULTD_EXECUTABLE_NAME} )
install( TARGETS
   ${BLOCKVAULTD_EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT base
)
