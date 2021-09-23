add_library( state_history_plugin
             state_history_plugin.cpp)

target_link_libraries( state_history_plugin state_history chain_plugin eosio_chain appbase ship_abi )
target_include_directories( state_history_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
