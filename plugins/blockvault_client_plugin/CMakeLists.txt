find_package(PkgConfig REQUIRED)

add_library( blockvault_client_plugin
             blockvault_client_plugin.cpp)
target_link_libraries( blockvault_client_plugin appbase eosio_chain)
target_include_directories( blockvault_client_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if (APPLE AND NOT DEFINED ENV{PKG_CONFIG_PATH})
   # to find libpq installed by homebrew, it is required to set the environment variable PKG_CONFIG_PATH
   set(ENV{PKG_CONFIG_PATH} /usr/local/opt/libpq/lib/pkgconfig)
endif(APPLE AND NOT DEFINED ENV{PKG_CONFIG_PATH})

pkg_check_modules(pqxx IMPORTED_TARGET libpqxx>=6.0 libpq>=10.0)

if (pqxx_FOUND)
   add_library(blockvault postgres_backend.cpp)
   target_include_directories(blockvault PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
   target_link_libraries(blockvault PUBLIC PkgConfig::pqxx eosio_chain)

   target_link_libraries( blockvault_client_plugin blockvault)
   target_compile_definitions(blockvault_client_plugin PRIVATE HAS_PQXX)

   add_subdirectory(tests)
endif(pqxx_FOUND)
