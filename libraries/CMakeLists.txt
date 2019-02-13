add_subdirectory( fc )
add_subdirectory( builtins )
add_subdirectory( softfloat )
add_subdirectory( chainbase )
add_subdirectory( wasm-jit )
add_subdirectory( appbase )
add_subdirectory( chain )
add_subdirectory( testing )

#turn tools&tests off; not needed for library build
set(BUILD_TESTS OFF CACHE BOOL "Build GTest-based tests")
set(BUILD_TOOLS OFF CACHE BOOL "Build wabt tools")
set(RUN_RE2C OFF CACHE BOOL "Run re2c")
set(WITH_EXCEPTIONS ON CACHE BOOL "Build with exceptions enabled" FORCE)
add_subdirectory( wabt )

set(ENABLE_STATIC ON)
set(CMAKE_MACOSX_RPATH OFF)
set(BUILD_ONLY_LIB ON CACHE BOOL "Library only build")
message(STATUS "Starting yubihsm configuration...")
add_subdirectory( yubihsm EXCLUDE_FROM_ALL )
set_target_properties(yubihsm_static PROPERTIES COMPILE_OPTIONS "-fno-lto")
message(STATUS "yubihsm configuration complete")

get_property(_CTEST_CUSTOM_TESTS_IGNORE GLOBAL PROPERTY CTEST_CUSTOM_TESTS_IGNORE)
set_property(GLOBAL PROPERTY CTEST_CUSTOM_TESTS_IGNORE
  "change_authkey import_ed decrypt_ec decrypt_rsa ssh logs generate_rsa import_ec echo\
  yubico_otp wrap_data wrap info import_rsa import_authkey generate_hmac generate_ec\
  attest pbkdf2 parsing ${_CTEST_CUSTOM_TESTS_IGNORE}")