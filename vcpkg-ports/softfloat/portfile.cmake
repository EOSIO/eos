set(SOFTFLOAT_VERSION 3e)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO eosio/berkeley-softfloat-3
    REF 44492cd324a16cf8faad854e6d8a141034c301c0
    SHA512 1c6ad4d70af03a01fdc915ec57596fe04ee3a0822de9bb7d31061e7bd8106c9344dc12f87da6c285399fe6c05549c6cac92f49c37fad4db516d21bc7c39ef3ae
    PATCHES fix-install.patch
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "share/softfloat")

file(WRITE ${CURRENT_PACKAGES_DIR}/share/${PORT}/softfloat-config.cmake [=[
include ("${CMAKE_CURRENT_LIST_DIR}/softfloat-targets.cmake")    
]=])

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL ${SOURCE_PATH}/COPYING.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
