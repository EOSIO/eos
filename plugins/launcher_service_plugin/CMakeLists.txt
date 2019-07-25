file(GLOB HEADERS "include/eosio/launcher_service_plugin/*.hpp")
add_library( launcher_service_plugin
             launcher_service_plugin.cpp
             httpc.cpp
             ${HEADERS} )

target_link_libraries( launcher_service_plugin appbase fc chain_plugin http_plugin appbase )
target_include_directories( launcher_service_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
