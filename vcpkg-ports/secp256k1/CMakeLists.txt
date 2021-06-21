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
        find_path(GMP_INCLUDE_DIR gmp.h PATHS "${_VCPKG_INSTALLED_DIR}/include" REQUIRED)
        find_library(GMP_LIBRARY_DEBUG NAMES libgmp  gmp  NAMES_PER_DIR PATH_SUFFIXES lib PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug" REQUIRED)
        find_library(GMP_LIBRARY_RELEASE NAMES libgmp gmp NAMES_PER_DIR PATH_SUFFIXES lib PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}" REQUIRED)
        include(SelectLibraryConfigurations)
        select_library_configurations(GMP)
        target_include_directories(secp256k1 PUBLIC "${GMP_INCLUDE_DIR}")
        target_link_libraries(secp256k1 PUBLIC ${GMP_LIBRARIES})
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