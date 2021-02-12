add_executable( eosio-chainid main.cpp )

target_link_libraries( eosio-chainid PRIVATE eosio_chain)

copy_bin( eosio-chainid )
install( TARGETS
   eosio-chainid RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT base
)
