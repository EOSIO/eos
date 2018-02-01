find_package(Intl REQUIRED)
find_package(Gperftools QUIET)

if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "eosc_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${eosc_BUILD_VERSION}")
  else()
    set(eosc_BUILD_VERSION 0)
  endif()
endif()

set(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
set(LOCALEDOMAIN eosioc)
configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

add_executable(eosioc-bin main.cpp help_text.cpp localize.hpp config.hpp CLI11.hpp eosioc_helper.cpp)
target_include_directories(eosioc-bin PUBLIC ${Intl_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(eosioc-bin PRIVATE
    appbase
    chain_api_plugin
    producer_plugin
    chain_plugin
    http_plugin
    eosio_chain
    fc
    eosioc
    ${CMAKE_DL_LIBS}
    ${PLATFORM_SPECIFIC_LIBS}
    ${Intl_LIBRARIES})
set_target_properties(eosioc-bin PROPERTIES OUTPUT_NAME eosioc)

install(TARGETS eosioc-bin
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
