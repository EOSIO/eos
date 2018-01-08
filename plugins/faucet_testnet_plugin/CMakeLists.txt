file(GLOB HEADERS "include/eosio/faucet_testnet_plugin/*.hpp")
add_library( faucet_testnet_plugin
             faucet_testnet_plugin.cpp
             ${HEADERS} )

target_link_libraries( faucet_testnet_plugin appbase fc http_plugin chain_plugin )
target_include_directories( faucet_testnet_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   faucet_testnet_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eosio/faucet_testnet_plugin" )
