file(GLOB HEADERS "include/eosio/trace_api_plugin/*.hpp")
add_library( trace_api_plugin
             request_handler.cpp
             store_provider.cpp
             abi_data_handler.cpp
             compressed_file.cpp
             trace_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( trace_api_plugin chain_plugin http_plugin eosio_chain appbase )
target_include_directories( trace_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_subdirectory( utils )
add_subdirectory( test )
