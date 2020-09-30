project(blockvault_cli_tst_net)
cmake_minimum_required( VERSION 3.8 )
message(STATUS "find OPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}, OPENSSL_LIBRARIES=${OPENSSL_LIBRARIES},OPENSSL_CRYPTO_LIBRARY=${OPENSSL_CRYPTO_LIBRARY},OPENSSL_SSL_LIBRARY=${OPENSSL_SSL_LIBRARY},OPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
find_package(Boost 1.70 )
message(STATUS "find BOOST_ROOT=${BOOST_ROOT},Boost_DIR=${Boost_DIR},")
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${Boost_DIR}/../../../include)
link_directories(${OPENSSL_ROOT_DIR}/lib)
#find_library(SSL_LIB libssl.a REQUIRED)
#find_library(CRYPTO_LIB libcrypto.a REQUIRED)
add_executable(t0001_hcs.run t0001_hcs.cpp)
add_executable(t0002_hcss.run t0002_hcss.cpp)
add_executable(t0003_wcs.run t0003_wcs.cpp)
add_executable(t0004_wcss.run t0004_wcss.cpp)
#message(STATUS "SSL_LIB=${SSL_LIB}, CRYPTO_LIB=${CRYPTO_LIB}")
target_link_libraries( t0002_hcss.run 
                        ${OPENSSL_SSL_LIBRARY}
                        ${OPENSSL_CRYPTO_LIBRARY} )
target_link_libraries( t0004_wcss.run 
                        ${OPENSSL_SSL_LIBRARY}
                        ${OPENSSL_CRYPTO_LIBRARY} )
#target_include_directories( blockvault_svr_tst_net PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
