# will be implictly used for any compilation unit if not overrided by SYSTEM_INCLUDE_FOLDERS parameter
# these directories go as -isystem <dir> to avoid warnings from code of third-party libraries
set(DEFAULT_SYSTEM_INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts/libc++/upstream/include ${CMAKE_SOURCE_DIR}/contracts/musl/upstream/include ${Boost_INCLUDE_DIR})

set(STANDARD_INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts)

add_subdirectory(eosiolib)
add_subdirectory(musl)
add_subdirectory(libc++)
add_subdirectory(multi_index_test)
add_subdirectory(eosio.system)
add_subdirectory(identity)
add_subdirectory(currency)
add_subdirectory(stltest)
add_subdirectory(exchange)

#add_subdirectory(bancor)
#add_subdirectory(eosio.system)
add_subdirectory(asserter)
add_subdirectory(infinite)
add_subdirectory(proxy)
add_subdirectory(test_api)
add_subdirectory(test_api_mem)
add_subdirectory(test_api_db)
add_subdirectory(test_api_multi_index)
#add_subdirectory(social)
add_subdirectory(test.system)
add_subdirectory(noop)

file(GLOB SKELETONS RELATIVE ${CMAKE_SOURCE_DIR}/contracts "skeleton/*")

# Documented multiple output support is broken, so properly setting up the multiple
# dependencies in the custom target is not possible.  (CMake 3.5)
add_custom_command(OUTPUT share/eosio/skeleton/skeleton.cpp
                   COMMAND ${CMAKE_COMMAND} -E make_directory ../share/eosio/skeleton
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/skeleton ../share/eosio/skeleton
                   DEPENDS ${SKELETONS}
                   COMMENT Copying skeleton contract...
                   VERBATIM)
add_custom_target(copy_skeleton_contract ALL DEPENDS share/eosio/skeleton/skeleton.cpp)

install(DIRECTORY eosiolib DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY musl DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY libc++ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY skeleton DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/eosio)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_DATAROOTDIR}/eosio)
