file(GLOB HEADERS "include/eosio/chain_plugin/*.hpp")
add_library( chain_plugin
             chain_plugin.cpp
             ${HEADERS} )

target_link_libraries( chain_plugin eosio_chain http_plugin appbase plugins_common)
target_include_directories( chain_plugin PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include"
    ${PLUGINS_COMMON_PATH}/include
    )
