file(GLOB HEADERS "include/eosio/validator_plugin/*.hpp")

add_library( validator_plugin
        validator_plugin.cpp
        ${HEADERS}
        )

target_link_libraries( validator_plugin chain_plugin appbase eosio_chain eos_utilities )
target_include_directories( validator_plugin
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" )
