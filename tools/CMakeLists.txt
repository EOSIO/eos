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
