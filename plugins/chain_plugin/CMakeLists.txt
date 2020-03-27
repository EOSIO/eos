file(GLOB HEADERS "include/eosio/chain_plugin/*.hpp")
add_library( chain_plugin
             chain_plugin.cpp
             ${HEADERS} )

if(EOSIO_ENABLE_DEVELOPER_OPTIONS)
    message(WARNING "EOSIO Developer Options are enabled; these are NOT supported")
    target_compile_definitions(chain_plugin PUBLIC EOSIO_DEVELOPER)
endif()

target_link_libraries( chain_plugin eosio_chain appbase )
target_include_directories( chain_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include")
