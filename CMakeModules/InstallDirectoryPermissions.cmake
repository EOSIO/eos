# Fix directory permissions after installation of header files (primarily).
macro(install_directory_permissions)
  cmake_parse_arguments(ARG "" "DIRECTORY" "" ${ARGN})
  set(dir ${ARG_DIRECTORY})
  install(DIRECTORY DESTINATION ${dir}
          DIRECTORY_PERMISSIONS OWNER_READ
                                OWNER_WRITE
                                OWNER_EXECUTE
                                GROUP_READ
                                GROUP_EXECUTE
                                WORLD_READ
                                WORLD_EXECUTE
  )
endmacro(install_directory_permissions)
