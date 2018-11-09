file(GLOB HEADERS "include/eosio/notify_plugin/*.hpp")
add_library( notify_plugin
             notify_plugin.cpp
             ${HEADERS} include/eosio/notify_plugin/notify_plugin.hpp)

target_link_libraries( notify_plugin chain_plugin eosio_chain appbase fc )
target_include_directories( notify_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )