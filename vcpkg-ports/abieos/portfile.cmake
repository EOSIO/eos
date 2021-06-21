vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO eosio/abieos
    REF a019c01f6afee613df68dcb89e9446077aa5b842
    SHA512 ccb5c7611a0d3819cbdf6575d691cd25337c739edaeb3e2204809abd5ad78d921ff493178da98e731d835d90d45629fe92fc137de1c4ac55b7da9a530529763b
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "share/abieos")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)