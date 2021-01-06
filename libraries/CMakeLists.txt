option(PORTABLE CACHE ON)               # rocksdb: don't use sse4.2
option(WITH_GFLAGS CACHE OFF)           # rocksdb: don't use gflags
option(WITH_TESTS CACHE OFF)            # rocksdb: don't build this
option(WITH_TOOLS CACHE OFF)            # rocksdb: don't build this
option(WITH_BENCHMARK_TOOLS CACHE OFF)  # rocksdb: don't build this
option(FAIL_ON_WARNINGS CACHE OFF)      # rocksdb: stop the madness: warnings change over time

#on Linux, rocksdb will monkey with CMAKE_INSTALL_PREFIX is this is on
set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OFF)
# rocksdb disables USE_RTTI for release build, which breaks
# exception handling on MacOS.
if(APPLE)
  set(USE_RTTI ON)
endif()

add_subdirectory( fc )
add_subdirectory( builtins )

# Suppress warnings on 3rdParty Library
add_definitions( -w )
add_subdirectory( softfloat )
remove_definitions( -w )

add_subdirectory( chainbase )
add_subdirectory( wasm-jit )
add_subdirectory( appbase )
add_subdirectory( chain )
add_subdirectory( testing )
add_subdirectory( version )
add_subdirectory( state_history )
add_subdirectory( abieos )

# Suppress warnings on 3rdParty Library
add_definitions( -w )
add_subdirectory( rocksdb )
remove_definitions( -w )

add_subdirectory( chain_kv )
add_subdirectory( se-helpers )

set(USE_EXISTING_SOFTFLOAT ON CACHE BOOL "use pre-exisiting softfloat lib")
set(ENABLE_TOOLS OFF CACHE BOOL "Build tools")
set(ENABLE_TESTS OFF CACHE BOOL "Build tests")
set(ENABLE_ADDRESS_SANITIZER OFF CACHE BOOL "Use address sanitizer")
set(ENABLE_UNDEFINED_BEHAVIOR_SANITIZER OFF CACHE BOOL "Use UB sanitizer")
set(ENABLE_PROFILE OFF CACHE BOOL "Enable for profile builds")
if(eos-vm IN_LIST EOSIO_WASM_RUNTIMES OR eos-vm-jit IN_LIST EOSIO_WASM_RUNTIMES)
add_subdirectory( eos-vm )
endif()

set(ENABLE_STATIC ON)
set(CMAKE_MACOSX_RPATH OFF)
set(BUILD_ONLY_LIB ON CACHE BOOL "Library only build")
message(STATUS "Starting yubihsm configuration...")
add_subdirectory( yubihsm EXCLUDE_FROM_ALL )
target_compile_options(yubihsm_static PRIVATE -fno-lto -fcommon)
message(STATUS "yubihsm configuration complete")

get_property(_CTEST_CUSTOM_TESTS_IGNORE GLOBAL PROPERTY CTEST_CUSTOM_TESTS_IGNORE)
set_property(GLOBAL PROPERTY CTEST_CUSTOM_TESTS_IGNORE
  "change_authkey import_ed decrypt_ec decrypt_rsa ssh logs generate_rsa import_ec echo\
  yubico_otp wrap_data wrap info import_rsa import_authkey generate_hmac generate_ec\
  attest pbkdf2 parsing ${_CTEST_CUSTOM_TESTS_IGNORE}")

find_package(OpenSSL REQUIRED)
option(AMQP-CPP_LINUX_TCP CACHE ON)
add_subdirectory( amqp-cpp EXCLUDE_FROM_ALL )
target_include_directories(amqpcpp PRIVATE "${OPENSSL_INCLUDE_DIR}")
