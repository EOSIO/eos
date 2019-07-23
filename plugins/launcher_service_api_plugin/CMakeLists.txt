file(GLOB HEADERS "include/eosio/launcher_service_api_plugin/*.hpp")
add_library( launcher_service_api_plugin
             launcher_service_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( launcher_service_api_plugin appbase fc launcher_service_plugin)
target_include_directories( launcher_service_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
