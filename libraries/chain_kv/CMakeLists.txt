file(GLOB_RECURSE HEADERS "include/*.hpp")

add_library( chain_kv INTERFACE )

target_link_libraries( chain_kv
                       INTERFACE fc rocksdb softfloat
                     )

target_include_directories( chain_kv
                            INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" 
                          )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})

add_subdirectory(unit_tests)
