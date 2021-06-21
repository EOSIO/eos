vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO eosio/fc
    REF e5f903fdf0e82b8a1281510dd40bd1d927f61098
    SHA512 ccfe0cd675fb5c9ffa59409a220f92cf481f69fc06205e96a7af49e6b44efa90c48fa87ee28b85bd1977b4d33fe09a99d080ac0303399f85e33ceb571dcd5fb7
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS -DSKIP_FC_TESTS=ON
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "share/fc")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL ${SOURCE_PATH}/LICENSE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
file(INSTALL ${SOURCE_PATH}/src/network/LICENSE.go DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})