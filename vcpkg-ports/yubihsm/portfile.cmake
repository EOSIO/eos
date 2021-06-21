
set(YUBIHSM_VERSION 2.2.0)

if(VCPKG_TARGET_IS_LINUX)
    message("${PORT} currently requires the libusb from the system package manager:\n\nThese can be installed via apt-get install libusb-1.0-0-dev on Ubuntu systems\n or via yum install libusbx-devel on CentOS systems.")
endif()

vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/Yubico/yubihsm-shell/archive/${YUBIHSM_VERSION}.tar.gz"
    FILENAME "yubihsm-shell-src.tar.gz"
    SHA512 0224a64145a39ace015d56e008804d7a179711915d389be6e3c4b0a0a14f2d3c95d0aceaf0b779e70fc124606b50a3d5886f8fcd11427499c140b504495f70c6
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    REF ${YUBIHSM_VERSION}
    PATCHES
      yubihsm.patch
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
