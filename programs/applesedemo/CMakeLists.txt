if(APPLE)
add_executable( applesedemo main.cpp r1_signature_compactor.cpp )


target_link_libraries( applesedemo
        PRIVATE fc ${PLATFORM_SPECIFIC_LIBS} 
        )

set_target_properties(applesedemo PROPERTIES LINK_FLAGS "-framework security -framework corefoundation")

#Demostration of signing automatically during build; you will need to change parameters for your signing credentials
#[[
add_custom_command(TARGET applesedemo POST_BUILD
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/sign.sh C5139C2C4D7FA071EFBFD86CE44B652631C9376A 5A4683969Z.one.block.applesedemo /Users/spoon/Library/MobileDevice/Provisioning\ Profiles/95813ad5-e880-432f-85c6-ade3b3298392.provisionprofile
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   VERBATIM
                  )
]]

install( TARGETS
   applesedemo

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

endif(APPLE)