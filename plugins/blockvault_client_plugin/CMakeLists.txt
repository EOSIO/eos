find_package(PkgConfig REQUIRED)

add_library( blockvault_client_plugin
             blockvault_client_plugin.cpp)
target_link_libraries( blockvault_client_plugin appbase eosio_chain)
target_include_directories( blockvault_client_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

set(ENV{PATH} "/usr/local/pgsql/bin:$ENV{PATH}")
if (APPLE AND EXISTS "/usr/local/pgsql/lib/pkgconfig")
   # to find libpq installed by homebrew, it is required to set the environment variable PKG_CONFIG_PATH
   if (DEFINED ENV{PKG_CONFIG_PATH})
      set(ENV{PKG_CONFIG_PATH} "/usr/local/pgsql/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
   else()
      set(ENV{PKG_CONFIG_PATH} "/usr/local/pgsql/lib/pkgconfig")
   endif()
elseif ( EXISTS "/usr/local/pgsql/lib/pkgconfig" )
   if (DEFINED ENV{PKG_CONFIG_PATH})
      set(ENV{PKG_CONFIG_PATH} "/usr/local/pgsql/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
   else()
      set(ENV{PKG_CONFIG_PATH} "/usr/local/pgsql/lib/pkgconfig")
   endif()
endif()

pkg_check_modules(pq IMPORTED_TARGET  libpq>=13.0)

if (pq_FOUND)
   add_library(blockvault postgres_backend.cpp)
   target_include_directories(blockvault PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
 
   add_library(pq STATIC IMPORTED)
   set_property(TARGET pq PROPERTY IMPORTED_LOCATION ${pq_STATIC_LIBRARY_DIRS}/libpq.a)
   set_property(TARGET pq PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${pq_STATIC_INCLUDE_DIRS})
   set(pq_lib pq)

   add_library(pgcom STATIC IMPORTED)
   set_property(TARGET pgcom PROPERTY IMPORTED_LOCATION ${pq_STATIC_LIBRARY_DIRS}/libpgcommon.a)
   set_property(TARGET pgcom PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${pq_STATIC_INCLUDE_DIRS})
   set(pgcom_lib pgcom)

   add_library(pgport STATIC IMPORTED)
   set_property(TARGET pgport PROPERTY IMPORTED_LOCATION ${pq_STATIC_LIBRARY_DIRS}/libpgport.a)
   set_property(TARGET pgport PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${pq_STATIC_INCLUDE_DIRS} )
   set(pgport_lib pgport)

   add_library(pqxx_lib STATIC IMPORTED)
   set_property(TARGET pqxx_lib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../../libraries/libpqxx/src/libpqxx.a)
   set_property(TARGET pqxx_lib PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/libpqxx/include ${CMAKE_CURRENT_BINARY_DIR}/../../libraries/libpqxx/include)
   target_link_libraries(blockvault PUBLIC pqxx_lib  ${pq_lib} ${pgcom_lib} ${pgport_lib} eosio_chain)

   target_link_libraries( blockvault_client_plugin blockvault)
   target_compile_definitions(blockvault_client_plugin PRIVATE HAS_PQXX)

   add_subdirectory(tests)
else()
   message(WARNING "libpqxx is not found, blockvault_client_plugin will not be functional for the lack of available backend")
endif()
