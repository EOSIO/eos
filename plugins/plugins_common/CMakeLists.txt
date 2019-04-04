file(GLOB HEADERS "include/eosio/plugins_common/*.hpp")
add_library(plugins_common
            chain_utils.cpp
            ${HEADERS} )

target_link_libraries(plugins_common eosio_chain appbase)
target_include_directories(plugins_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
