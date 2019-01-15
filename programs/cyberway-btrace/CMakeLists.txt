
add_executable( ${BTRACE_EXECUTABLE_NAME} main.cpp)
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

IF(APPLE)
  ADD_COMPILE_FLAG("-D_GNU_SOURCE")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "btrace_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${btrace_BUILD_VERSION}")
  else()
    set(btrace_BUILD_VERSION 0)
  endif()
else()
  set(btrace_BUILD_VERSION 0)
endif()

find_package(Intl REQUIRED)

set(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
set(LOCALEDOMAIN ${BTRACE_EXECUTABLE_NAME})

target_include_directories(${BTRACE_EXECUTABLE_NAME} PUBLIC ${Intl_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( ${BTRACE_EXECUTABLE_NAME}
                       PRIVATE fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES} )
                    

copy_bin( ${BTRACE_EXECUTABLE_NAME} )
install( TARGETS
   ${BTRACE_EXECUTABLE_NAME}

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
