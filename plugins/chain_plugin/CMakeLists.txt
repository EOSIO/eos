file(GLOB HEADERS "include/eosio/chain_plugin/*.hpp")
add_library( chain_plugin
             account_query_db.cpp
             chain_plugin.cpp
             ${HEADERS} )

target_link_libraries( chain_plugin eosio_chain appbase )
target_include_directories( chain_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include")
