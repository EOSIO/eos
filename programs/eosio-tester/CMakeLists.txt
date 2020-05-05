add_executable( ${TESTER_EXECUTABLE_NAME} main.cpp )
if( UNIX AND NOT APPLE )
    set(rt_library rt )
endif()

target_link_libraries( ${TESTER_EXECUTABLE_NAME}
  PRIVATE rodeos_lib state_history eosio_chain ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories(${TESTER_EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

copy_bin( ${TESTER_EXECUTABLE_NAME} )
install( TARGETS
  ${TESTER_EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
