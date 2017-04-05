file(GLOB HEADERS "include/eos/app/*.hpp")
file(GLOB EGENESIS_HEADERS "../egenesis/include/eos/app/*.hpp")

add_library( eos_app 
             api.cpp
             application.cpp
             plugin.cpp
             ${HEADERS}
             ${EGENESIS_HEADERS}
           )

# need to link eos_debug_producer because plugins aren't sufficiently isolated #246
target_link_libraries( eos_app eos_chain fc eos_net eos_utilities eos_debug_producer )
target_include_directories( eos_app
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include" )

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   eos_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/eos/app" )
