file(GLOB HEADERS "include/eosio/signed_validation_plugin/*.hpp")
add_library( signed_validation_plugin
             signed_validation_plugin.cpp
             ${HEADERS} )
             
target_link_libraries( signed_validation_plugin appbase fc chain_plugin )
target_include_directories( signed_validation_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                            signed_validation_plugin PUBLIC ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            signed_validation_plugin PUBLIC ${CMAKE_BINARY_DIR}/unittests/include )
