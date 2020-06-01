file(GLOB HEADERS "include/eosio/state-history/*.hpp")

add_library( state_history
             abi.cpp
             compression.cpp
             create_deltas.cpp
             log.cpp
             trace_converter.cpp
             ${HEADERS}
           )

target_link_libraries( state_history 
                       PUBLIC eosio_chain fc chainbase softfloat
                     )

target_include_directories( state_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                          )

if(ENABLE_UNITY_BUILD)
   set_target_properties(state_history PROPERTIES UNITY_BUILD TRUE)
endif() 
