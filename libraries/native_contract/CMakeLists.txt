file(GLOB HEADERS "include/eos/native_contract/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( eos_native_contract
             eos_contract.cpp
             native_contract_chain_initializer.cpp
             native_contract_chain_administrator.cpp
             genesis_state.cpp

             ${HEADERS}
           )

target_link_libraries( eos_native_contract eos_chain fc )
target_include_directories( eos_native_contract
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   eos_native_contract

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/eos/native_contract" )
