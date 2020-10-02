project(blockvault_client)
cmake_minimum_required( VERSION 3.8 )
message(STATUS "find OPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}, OPENSSL_LIBRARIES=${OPENSSL_LIBRARIES},OPENSSL_CRYPTO_LIBRARY=${OPENSSL_CRYPTO_LIBRARY},OPENSSL_SSL_LIBRARY=${OPENSSL_SSL_LIBRARY},OPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
find_package(Boost 1.70 )
message(STATUS "find BOOST_ROOT=${BOOST_ROOT},Boost_DIR=${Boost_DIR},")
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${Boost_DIR}/../../../include)
#include_directories("include/eosio/blockvault_client")
link_directories(${OPENSSL_ROOT_DIR}/lib)

file(GLOB HEADERS "include/eosio/blockvault_client/*.hpp")
add_library( connect_mgr
             connect_mgr.cpp
             ${HEADERS} )

target_link_libraries( connect_mgr  ${OPENSSL_SSL_LIBRARY}  ${OPENSSL_CRYPTO_LIBRARY} pthread  )
target_include_directories( connect_mgr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_subdirectory(test)
