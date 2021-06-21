vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO eosio/appbase
    REF d0472ea03df5f47f685dbaad7e52882b57e52b20
    SHA512 f658e22f3ea4678df6fe6192f2132934aa85c90848bbad4386d235ed87abfe39201bfcf7e5b2ed752f61ecb07065cf798aa5d93a95caac213c550b19298d86bd
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "share/appbase")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL ${SOURCE_PATH}/LICENSE.md DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)