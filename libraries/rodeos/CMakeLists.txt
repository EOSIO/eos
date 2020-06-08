file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

add_library( rodeos_lib
             embedded_rodeos.cpp
             rodeos.cpp
             wasm_ql.cpp
             ${HEADERS}
           )

target_link_libraries( rodeos_lib 
                       PUBLIC abieos chain_kv eosio_chain fc softfloat
                     )

target_include_directories( rodeos_lib
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                          )

if(ENABLE_UNITY_BUILD)
   set_target_properties(rodeos_lib PROPERTIES UNITY_BUILD TRUE)
endif() 
