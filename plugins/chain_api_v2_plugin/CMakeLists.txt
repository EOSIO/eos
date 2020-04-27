add_library(chain_api_v2_plugin chain_api_v2_plugin.cpp)

target_link_libraries(chain_api_v2_plugin chain_plugin http_plugin appbase)
target_include_directories(chain_api_v2_plugin
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
