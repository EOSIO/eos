file(GLOB HEADERS "include/eosio/statetrack_plugin/*.hpp")
add_library( statetrack_plugin
             statetrack_plugin.cpp
             ${HEADERS} )

target_link_libraries( statetrack_plugin chain_plugin appbase fc )
target_include_directories( statetrack_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )