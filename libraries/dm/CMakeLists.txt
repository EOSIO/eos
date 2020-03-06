file(GLOB HEADERS "include/dfuse/dm/*.hpp" )

## SORT .cpp by most likely to change / break compile
add_library( dfuse_dm
   log_appender.cpp
   ${HEADERS}
)

target_link_libraries( dfuse_dm fc Logging )
target_include_directories( dfuse_dm
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
)

install( TARGETS dfuse_dm
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dfuse/dm/
      DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/dfuse/dm
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" EXCLUDE
)
