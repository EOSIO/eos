#find_package(PkgConfig REQUIRED)
#pkg_search_module(HIDAPI REQUIRED hidapi)
#add_subdirectory(tinycbor)

add_executable(eosio-webauthn-create eosio-webauthn-create.cpp)
target_link_libraries(eosio-webauthn-create microfido)
#XXX Use imported target on >3.5 upgrade
#target_include_directories(eosio-webauthn-create PUBLIC ${HIDAPI_INCLUDE_DIRS})
#target_link_libraries(eosio-webauthn-create ${HIDAPI_LIBRARIES} tinycbor fc)
#target_compile_options(eosio-webauthn-create PUBLIC ${HIDAPI_CFLAGS_OTHER})
#target_link_directories(eosio-webauthn-create PUBLIC ${HIDAPI_LIBRARY_DIRS})
