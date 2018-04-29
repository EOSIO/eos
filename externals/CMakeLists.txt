add_subdirectory( binaryen )
add_subdirectory( soci/src )
install(DIRECTORY magic_get/include/boost DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
