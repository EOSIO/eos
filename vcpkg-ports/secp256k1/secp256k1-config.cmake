include(CMakeFindDependencyMacro)
find_dependency(gmp)
include("${CMAKE_CURRENT_LIST_DIR}/secp256k1-targets.cmake")
