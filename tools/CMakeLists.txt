configure_file( eosiocpp.in eosiocpp @ONLY)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/eosiocpp DESTINATION bin/ PERMISSIONS WORLD_EXECUTE OWNER_EXECUTE GROUP_EXECUTE WORLD_READ GROUP_READ OWNER_READ )

add_executable( print_floats print_floats.cpp )
target_link_libraries( print_floats PRIVATE ${Boost_LIBRARIES} )
