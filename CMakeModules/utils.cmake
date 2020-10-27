macro( copy_bin file )
   add_custom_command( TARGET ${file} POST_BUILD COMMAND mkdir -p ${CMAKE_BINARY_DIR}/bin )
   add_custom_command( TARGET ${file} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${file}${CMAKE_EXECUTABLE_SUFFIX} ${CMAKE_BINARY_DIR}/bin/ )
endmacro( copy_bin )

function(create_mono_lib INPUT OUTPUT)
   set(CMDS "")
   set(DEPS "")
   get_target_property(LINKED ${INPUT} LINK_LIBRARIES)
   FOREACH(LIB ${LINKED})
      if(TARGET ${LIB})
         get_target_property(LIB_TYPE ${LIB} TYPE)
         message(STATUS "LL ${LIB} ${LIB_TYPE}")
         set(CMDS "${CMDS} COMMAND ar -x $<TARGET_FILE:${LIB}> ")
         set(DEPS "${DEPS} ${LIB}")
      endif()
   ENDFOREACH()
   add_custom_target(${OUTPUT}-combined
      ${CMDS}
      COMMAND ar -qcs ${CMAKE_BINARY_DIR}/lib${OUTPUT}.a *.o
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${DEPS}
   )
   
   add_library(${OUTPUT} STATIC IMPORTED GLOBAL)
   add_dependencies(${OUTPUT} ${OUTPUT}-combined)
   set_target_properties(${OUTPUT}
      PROPERTIES
      IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib${OUTPUT}.a
   )
   message(STATUS "KLOISFD ${CMDS}")
endfunction()
