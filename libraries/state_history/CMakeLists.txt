file(GLOB HEADERS "include/eosio/state-history/*.hpp")

add_library( state_history
             abi.cpp
             ${HEADERS}
           )

target_include_directories( state_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                          )
