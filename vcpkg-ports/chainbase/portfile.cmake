vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO eosio/chainbase
    REF e1e0d2418f881f293b03588828ac22ec757d2a1c
    SHA512 233be9fed3426e41ee4c3cd00b7d31c3f6a06296fe5dbd8e9b70b97ec0f79bfdf2b52219581d7ecfa3c9ff3fe6dca867e2cfea78c499549ba5c62a4b1168fe84
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "share/chainbase")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL ${SOURCE_PATH}/LICENSE.md DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)