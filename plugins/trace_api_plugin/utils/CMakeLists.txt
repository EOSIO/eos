add_executable( trace_api_util
        compress_cmd.cpp
        trace_api_util.cpp
)
target_link_libraries( trace_api_util trace_api_plugin )
target_include_directories( trace_api_util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

copy_bin( trace_api_util )
install( TARGETS
    trace_api_util

    COMPONENT base

    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
