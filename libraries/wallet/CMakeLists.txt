file(GLOB HEADERS "include/eos/wallet/*.hpp")

add_library( eos_wallet
             wallet.cpp

             ${HEADERS}
           )
target_link_libraries( eos_wallet fc eos_chain )
target_include_directories( eos_wallet
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   eos_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/eos/wallet" )
