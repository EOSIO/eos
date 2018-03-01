file(GLOB HEADERS "include/eosio/debug_plugin/*.hpp")
add_library( debug_plugin
             debug_plugin.cpp
#             ../../contracts/noop/noop.cpp
             ../../contracts/currency/currency.cpp
#             ../../contracts/test_api/test_api.cpp
#             ../../contracts/test_api_db/test_api_db.cpp
             ${HEADERS} )

target_link_libraries( debug_plugin appbase fc chain_plugin eosio_chain )
target_include_directories( debug_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts" )
#set_source_files_properties(../../contracts/noop/noop.cpp PROPERTIES COMPILE_FLAGS -DEOSIO_NATIVE_CONTRACT_COMPILATION)
#set_source_files_properties(../../contracts/currency/currency.cpp PROPERTIES COMPILE_FLAGS -DEOSIO_NATIVE_CONTRACT_COMPILATION)
target_compile_definitions(debug_plugin PRIVATE EOSIO_NATIVE_CONTRACT_COMPILATION)

install( TARGETS
   debug_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eosio/debug_plugin" )
