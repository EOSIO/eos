project(blockvault_svr_tst_net)
cmake_minimum_required( VERSION 3.8 )
message(STATUS "find OPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}, OPENSSL_LIBRARIES=${OPENSSL_LIBRARIES},OPENSSL_CRYPTO_LIBRARY=${OPENSSL_CRYPTO_LIBRARY},OPENSSL_SSL_LIBRARY=${OPENSSL_SSL_LIBRARY},OPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
find_package(Boost 1.70 )
message(STATUS "find BOOST_ROOT=${BOOST_ROOT},Boost_DIR=${Boost_DIR},")
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${Boost_DIR}/../../../include)
link_directories(${OPENSSL_ROOT_DIR}/lib)
#find_library(SSL_LIB libssl.a REQUIRED)
#find_library(CRYPTO_LIB libcrypto.a REQUIRED)
add_executable(t0001_hss.run t0001_hss.cpp)
add_executable(t0002_hsss.run t0002_hsss.cpp)
add_executable(t0003_wss.run t0003_wss.cpp)
add_executable(t0004_wsss.run t0004_wsss.cpp)
message(STATUS "SSL_LIB=${SSL_LIB}, CRYPTO_LIB=${CRYPTO_LIB}")
target_link_libraries( t0002_hsss.run 
                        ${OPENSSL_SSL_LIBRARY}
                        ${OPENSSL_CRYPTO_LIBRARY} )
target_link_libraries( t0004_wsss.run 
                        ${OPENSSL_SSL_LIBRARY}
                        ${OPENSSL_CRYPTO_LIBRARY} )
#target_include_directories( blockvault_svr_tst_net PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
