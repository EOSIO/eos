
set(YUBIHSM_VERSION 2.2.0)

if(VCPKG_TARGET_IS_LINUX)
    message("${PORT} currently requires the libusb from the system package manager:\n\nThese can be installed via apt-get install libusb-1.0-0-dev on Ubuntu systems\n or via yum install libusbx-devel on CentOS systems.")
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Yubico/yubihsm-shell
    REF 2.2.0
    SHA512 e4a77fbf920a826ce3df5552ef94e329bca3e1e26bdb5cb0b80e30fabbd0cdc8c338d11cae3d00ec3b00d7b4688f9fad8ded64d42bfcf09df393a7a75a5ec551
    PATCHES yubihsm.patch
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS 
      -DENABLE_STATIC=ON
      -DBUILD_ONLY_LIB=ON 
      -DDISABLE_LTO=ON
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "share/yubihsm")
file(INSTALL
    ${CMAKE_CURRENT_LIST_DIR}/yubihsm-config.cmake
    DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT}
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
