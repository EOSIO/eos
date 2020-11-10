find_package(PkgConfig REQUIRED)

pkg_check_modules(pqxx REQUIRED IMPORTED_TARGET libpqxx>=6.0 libpq>=10.0)

if (pqxx_FOUND)

add_library(blockvault postgres_backend.cpp)
target_include_directories(blockvault PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(blockvault PUBLIC PkgConfig::pqxx eosio_chain)


add_library( blockvault_client_plugin
             blockvault_client_plugin.cpp)

target_link_libraries( blockvault_client_plugin appbase blockvault)
target_include_directories( blockvault_client_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_subdirectory(tests)

endif(pqxx_FOUND)
