option(PORTABLE CACHE ON)               # rocksdb: don't use sse4.2
option(WITH_GFLAGS CACHE OFF)           # rocksdb: don't use gflags
option(WITH_TESTS CACHE OFF)            # rocksdb: don't build this
option(WITH_TOOLS CACHE OFF)            # rocksdb: don't build this
option(WITH_BENCHMARK_TOOLS CACHE OFF)  # rocksdb: don't build this
option(FAIL_ON_WARNINGS CACHE OFF)      # rocksdb: stop the madness: warnings change over time

#on Linux, rocksdb will monkey with CMAKE_INSTALL_PREFIX is this is on
set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OFF)

add_subdirectory( fc )
add_subdirectory( builtins )
add_subdirectory( softfloat )
add_subdirectory( chainbase )
add_subdirectory( wasm-jit )
add_subdirectory( appbase )
add_subdirectory( chain )
add_subdirectory( testing )
add_subdirectory( version )
add_subdirectory( state_history )
add_subdirectory( abieos )
add_subdirectory( rocksdb EXCLUDE_FROM_ALL )
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

add_subdirectory( rodeos )

find_package(OpenSSL REQUIRED)
option(AMQP-CPP_LINUX_TCP CACHE ON)
add_subdirectory( amqp-cpp EXCLUDE_FROM_ALL )
target_include_directories(amqpcpp PRIVATE "${OPENSSL_INCLUDE_DIR}")


find_package(Boost 1.70 REQUIRED COMPONENTS system)

add_library(nuraft STATIC 
    nuraft/src/asio_service.cxx
    nuraft/src/buffer.cxx
    nuraft/src/buffer_serializer.cxx
    nuraft/src/cluster_config.cxx
    nuraft/src/crc32.cxx
    nuraft/src/error_code.cxx
    nuraft/src/handle_append_entries.cxx
    nuraft/src/handle_client_request.cxx
    nuraft/src/handle_custom_notification.cxx
    nuraft/src/handle_commit.cxx
    nuraft/src/handle_join_leave.cxx
    nuraft/src/handle_priority.cxx
    nuraft/src/handle_snapshot_sync.cxx
    nuraft/src/handle_timeout.cxx
    nuraft/src/handle_user_cmd.cxx
    nuraft/src/handle_vote.cxx
    nuraft/src/launcher.cxx
    nuraft/src/peer.cxx
    nuraft/src/raft_server.cxx
    nuraft/src/snapshot.cxx
    nuraft/src/snapshot_sync_req.cxx
    nuraft/src/srv_config.cxx
    nuraft/src/stat_mgr.cxx)

target_include_directories(nuraft PRIVATE ${Boost_INCLUDE_DIR}/boost nuraft/include/libnuraft PUBLIC nuraft/include)
target_compile_definitions(nuraft PRIVATE USE_BOOST_ASIO)
target_link_libraries(nuraft Boost::system OpenSSL::SSL)