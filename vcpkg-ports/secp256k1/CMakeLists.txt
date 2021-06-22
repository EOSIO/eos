cmake_minimum_required(VERSION 3.9)
project(secp256k1 C)

option(INSTALL_HEADERS "Install header files" ON)

file(GLOB SOURCES src/secp256k1.c)
add_library(secp256k1 ${SOURCES})

target_include_directories(secp256k1 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" $<INSTALL_INTERFACE:include>)

target_compile_definitions(secp256k1 PRIVATE HAVE_CONFIG_H)

string(REPLACE "," ";" FEATURES ${FEATURES})
foreach (feature  ${FEATURES})
    if ("${feature}" MATCHES "^module-([a-z0-9]+)")
        string(TOUPPER ${CMAKE_MATCH_1} MODULE_NAME)
        target_compile_definitions(secp256k1 PRIVATE ENABLE_MODULE_${MODULE_NAME})
	elseif("${feature}" STREQUAL gmp)
        target_compile_definitions(secp256k1 PRIVATE HAVE_LIBGMP)
        find_package(gmp CONFIG REQUIRED)
        target_link_libraries(secp256k1 PUBLIC gmp::gmp)
    endif()
endforeach()

if(INSTALL_HEADERS)
    file(GLOB HEADERS include/*.h)
    install(FILES ${HEADERS} DESTINATION include)
endif()

install(TARGETS secp256k1 EXPORT secp256k1-targets
    ARCHIVE
    LIBRARY
)

install(
    EXPORT secp256k1-targets
    FILE secp256k1-targets.cmake
    DESTINATION share/secp256k1
)