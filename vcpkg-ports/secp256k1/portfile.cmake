vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO "bitcoin-core/secp256k1"
    REF "b61f9da54eff6d8137e0681b403b48be62f0460a"
    SHA512 fb6486249c4adfdd74e6ac435ca1f001cdda67f350c17997e374303cd2b5d4a3e6b70638957cd51112b566f969b17b00195bded22fa4179d2d9096ab213cad94
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/libsecp256k1-config.h DESTINATION ${SOURCE_PATH})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})
string(REPLACE ";" "," FEATURES "${FEATURES}")

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS -DFEATURES="${FEATURES}"
    OPTIONS_DEBUG
        -DINSTALL_HEADERS=OFF
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH share/${PORT} TARGET_PATH share/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
configure_file(${CMAKE_CURRENT_LIST_DIR}/secp256k1-config.cmake ${CURRENT_PACKAGES_DIR}/share/${PORT}/secp256k1-config.cmake @ONLY)
file(INSTALL ${SOURCE_PATH}/COPYING DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
