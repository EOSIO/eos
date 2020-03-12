file(GLOB HEADERS "include/eosio/tester_module/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( eosio_tester_module SHARED
             base.cpp
             database.cpp
             ${HEADERS}
        )

target_link_libraries( eosio_tester_module
   PRIVATE eosio_chain
   PRIVATE fc
   PRIVATE chainbase
   PRIVATE Logging
   PRIVATE IR
   PRIVATE WAST
   PRIVATE WASM
   PRIVATE Runtime
   PRIVATE eosio_version
   PRIVATE eosio_libmodule)

target_include_directories( eosio_tester_module
   PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
           "${CMAKE_CURRENT_BINARY_DIR}/include"
           )

install( TARGETS eosio_tester_module
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/tester
)

install_directory_permissions( DIRECTORY ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/tester )
