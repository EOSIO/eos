configure_file( eosiocpp.in eosiocpp @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/llvm-gcov.sh ${CMAKE_CURRENT_BINARY_DIR}/llvm-gcov.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ctestwrapper.sh ${CMAKE_CURRENT_BINARY_DIR}/ctestwrapper.sh COPYONLY)
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
