file(GLOB UNIT_TESTS "*.cpp")

add_executable( net_plugin_tests ${UNIT_TESTS} )

target_link_libraries( net_plugin_tests
        net_plugin
        chain_plugin
        producer_plugin
        appbase
        fc
        ${PLATFORM_SPECIFIC_LIBS}
        eosio_testing
        eosio_chain
        )
target_include_directories( net_plugin_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/plugins/chain_interface/include
        ${CMAKE_SOURCE_DIR}/libraries/appbase/include
        ${CMAKE_SOURCE_DIR}/plugins/net_plugin/include
        ${CMAKE_SOURCE_DIR}/plugins/chain_plugin/include
        ${CMAKE_SOURCE_DIR}/libraries/testing/include
        ${CMAKE_SOURCE_DIR}/cmake-build-debug/unittests/include
        )

add_test(NAME net_plugin_tests COMMAND plugins/net_plugin/test/net_plugin_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
