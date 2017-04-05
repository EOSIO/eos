
file(GLOB headers "include/eos/utilities/*.hpp")

set(sources deterministic_openssl_rand.cpp
            ${headers})

add_library( deterministic_openssl_rand 
             ${sources}
             ${HEADERS} )
target_link_libraries( deterministic_openssl_rand fc )
target_include_directories( deterministic_openssl_rand
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../blockchain/include"
                          )

if (USE_PCH)
  set_target_properties(deterministic_openssl_rand PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(deterministic_openssl_rand)
endif(USE_PCH)

install( TARGETS
   deterministic_openssl_rand

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${headers} DESTINATION "include/eos/deterministic_openssl_rand" )
