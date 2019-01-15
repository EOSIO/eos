file(GLOB HEADERS "include/eosio/event_engine_plugin/*.hpp")
add_library( event_engine_plugin
             event_engine_plugin.cpp
             ${HEADERS} )

target_link_libraries( event_engine_plugin 
    PUBLIC chain_plugin appbase fc )
target_include_directories( event_engine_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
