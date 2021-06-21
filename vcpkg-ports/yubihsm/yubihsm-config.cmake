include(CMakeFindDependencyMacro)
find_dependency(OpenSSL)
find_dependency(CURL CONFIG)
find_dependency(PkgConfig)
pkg_search_module (LIBUSB REQUIRED libusb-1.0)

include ("${CMAKE_CURRENT_LIST_DIR}/yubihsm-targets.cmake")
