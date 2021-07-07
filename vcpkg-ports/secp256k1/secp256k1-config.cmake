include(CMakeFindDependencyMacro)
find_dependency(PkgConfig)
pkg_check_modules(GMP gmp IMPORTED_TARGET)
include("${CMAKE_CURRENT_LIST_DIR}/secp256k1-targets.cmake")
