file(GLOB HEADERS "include/eos/p2p_plugin/*.hpp")

add_library( p2p_plugin
             p2p_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( p2p_plugin chain_plugin appbase eos_chain eos_utilities eos_net )
target_include_directories( p2p_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   p2p_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
