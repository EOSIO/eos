file(GLOB HEADERS "include/eosio/chain_plugin/*.hpp")
add_library( chain_plugin
             account_query_db.cpp
             chain_plugin.cpp
             ${HEADERS} )

if(EOSIO_ENABLE_DEVELOPER_OPTIONS)
    message(WARNING "EOSIO Developer Options are enabled; these are NOT supported")
    target_compile_definitions(chain_plugin PUBLIC EOSIO_DEVELOPER)
endif()

target_link_libraries( chain_plugin eosio_chain appbase resource_monitor_plugin blockvault_client_plugin )
target_include_directories( chain_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include" "${CMAKE_CURRENT_SOURCE_DIR}/../resource_monitor_plugin/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/abieos/include")

add_subdirectory( test )
