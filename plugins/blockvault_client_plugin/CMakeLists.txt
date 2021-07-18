add_library( blockvault_client_plugin
             blockvault_client_plugin.cpp)
target_link_libraries( blockvault_client_plugin appbase eosio_chain)
target_include_directories( blockvault_client_plugin PUBLIC include )

if(NOT TARGET pqxx)
   message(WARNING "libpq not found, blockvault_client_plugin will not be functional for the lack of available backend")
   return()
endif()

add_library(blockvault postgres_backend.cpp)
target_include_directories(blockvault PUBLIC include)
target_link_libraries(blockvault PUBLIC pqxx eosio_chain)

target_link_libraries( blockvault_client_plugin blockvault)
target_compile_definitions(blockvault_client_plugin PRIVATE HAS_PQXX)

add_subdirectory(tests)
