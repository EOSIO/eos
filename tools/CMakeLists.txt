configure_file( eosiocpp.in eosiocpp @ONLY)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/eosiocpp DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
         PERMISSIONS OWNER_READ
                     OWNER_WRITE
                     OWNER_EXECUTE
                     GROUP_READ
                     GROUP_EXECUTE
                     WORLD_READ
                     WORLD_EXECUTE
)

add_executable( print_floats print_floats.cpp )
target_include_directories( print_floats PRIVATE ${Boost_INCLUDE_DIR} )
target_link_libraries( print_floats PRIVATE ${Boost_LIBRARIES} )
