file(GLOB HEADERS "include/eosio/resource_monitor_plugin/*.hpp")
add_library( resource_monitor_plugin
             resource_monitor_plugin.cpp
             system_file_space_provider.cpp
             resmon_impl.cpp
             ${HEADERS} )

target_link_libraries( resource_monitor_plugin appbase fc chain_plugin)
target_include_directories( resource_monitor_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_subdirectory( test )
