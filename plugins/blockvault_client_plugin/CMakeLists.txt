find_package(PkgConfig REQUIRED)

add_library( blockvault_client_plugin
             blockvault_client_plugin.cpp)
target_link_libraries( blockvault_client_plugin appbase eosio_chain)
target_include_directories( blockvault_client_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )


if (APPLE AND EXISTS "/usr/local/opt/libpq/lib/pkgconfig")
   # to find libpq installed by homebrew, it is required to set the environment variable PKG_CONFIG_PATH
   if (DEFINED ENV{PKG_CONFIG_PATH})
      set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libpq/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
   else()
      set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libpq/lib/pkgconfig")
   endif()
endif()

pkg_check_modules(pq IMPORTED_TARGET  libpq>=10.0)
pkg_check_modules(pqxx IMPORTED_TARGET libpqxx>=6.0)

if (pqxx_FOUND)
   add_library(blockvault postgres_backend.cpp)
   target_include_directories(blockvault PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include 
                                         PRIVATE ${pqxx_STATIC_INCLUDE_DIRS})

   target_link_libraries(blockvault PUBLIC libpqxx.a PkgConfig::pq eosio_chain)
   target_link_directories(blockvault PUBLIC ${pqxx_STATIC_LIBRARY_DIRS})

   target_link_libraries( blockvault_client_plugin blockvault)
   target_compile_definitions(blockvault_client_plugin PRIVATE HAS_PQXX)

   add_subdirectory(tests)
else()
   message(WARNING "libpqxx is not found, blockvault_client_plugin will not be functional for the lack of available backend")
endif()
