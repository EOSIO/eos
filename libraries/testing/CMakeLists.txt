file(GLOB HEADERS "include/eosio/testing/*.hpp")

configure_file(contracts.hpp.in include/testing_contracts/contracts.hpp ESCAPE_QUOTES)
add_library(eosio_testing_contracts INTERFACE)
target_include_directories(eosio_testing_contracts INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include/testing_contracts)

## SORT .cpp by most likely to change / break compile
add_library( eosio_testing
             tester.cpp
             tester_network.cpp
             ${HEADERS}
        )

target_link_libraries( eosio_testing eosio_testing_contracts eosio_chain fc chainbase Logging IR WAST WASM Runtime )
target_include_directories( eosio_testing
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                                   "${CMAKE_BINARY_DIR}/unittests/include"
                            )

set_target_properties( eosio_testing PROPERTIES PUBLIC_HEADER "${HEADERS}" )
install( TARGETS eosio_testing
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/testing
)
install_directory_permissions( DIRECTORY ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/testing )

