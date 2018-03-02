file(GLOB HEADERS "include/eos/chain_plugin/*.hpp")
add_library( chain_api_plugin
             chain_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( chain_api_plugin chain_plugin http_plugin appbase )
target_include_directories( chain_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
