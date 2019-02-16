
include(ProcessorCount)
ProcessorCount(NUM_CORES)

set(BOOST_ROOT "${CMAKE_CURRENT_BINARY_DIR}/boost_install")
set(MONGO_DRIVER_ROOT "${CMAKE_CURRENT_BINARY_DIR}/mongo_driver_install")
set(WASM_ROOT "${CMAKE_CURRENT_BINARY_DIR}/wasm_install")

set(CMAKE_LIST_CONTENT "
  cmake_minimum_required(VERSION 3.5)
  include(ExternalProject)

  ExternalProject_add(  boost
    PREFIX              boost
    URL                 https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2
    URL_HASH            SHA256=2684c972994ee57fc5632e03bf044746f6eb45d4920c343937a465fd67a5adba
    BUILD_IN_SOURCE     1
    CONFIGURE_COMMAND   ./bootstrap.sh --prefix=${BOOST_ROOT}
    BUILD_COMMAND       ./b2 -j${NUM_CORES}
    INSTALL_COMMAND     ./b2 install
  )

  ExternalProject_add(  mongo-c-driver
    PREFIX              mongo-c-driver
    GIT_REPOSITORY      https://github.com/mongodb/mongo-c-driver
    GIT_TAG             1.13.1
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    BUILD_COMMAND       cmake --build . -- -j ${NUM_CORES}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${MONGO_DRIVER_ROOT}
      -DCMAKE_BUILD_TYPE=Release
      -DENABLE_STATIC=ON
      -DENABLE_SSL=OPENSSL
      -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF
  )

  ExternalProject_add(  mongo-cxx-driver
    PREFIX              monngo-cxx-driver
    DEPENDS             mongo-c-driver
    GIT_REPOSITORY      https://github.com/mongodb/mongo-cxx-driver
    GIT_TAG             origin/releases/stable
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    BUILD_COMMAND       cmake --build . -- -j ${NUM_CORES}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${MONGO_DRIVER_ROOT}
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_SHARED_LIBS=OFF
  )

  ExternalProject_add(  llvm_clone
    PREFIX              llvm
    SOURCE_DIR          llvm/src/llvm
    GIT_REPOSITORY      https://github.com/llvm-mirror/llvm
    GIT_TAG             release_40
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    CONFIGURE_COMMAND   \"\"
    BUILD_COMMAND       \"\"
    INSTALL_COMMAND     \"\"
  )

  ExternalProject_add(  clang_clone
    DEPENDS             llvm_clone
    PREFIX              llvm
    SOURCE_DIR          llvm/src/llvm/tools/clang
    GIT_REPOSITORY      https://github.com/llvm-mirror/clang
    GIT_TAG             release_40
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    CONFIGURE_COMMAND   \"\"
    BUILD_COMMAND       \"\"
    INSTALL_COMMAND     \"\"
  )

  ExternalProject_add(  llvm_build
    DEPENDS             clang_clone
    PREFIX              llvm
    SOURCE_DIR          llvm/src/llvm
    DOWNLOAD_COMMAND    \"\"
    UPDATE_COMMAND      \"\"
    BUILD_COMMAND       cmake --build . -- -j ${NUM_CORES}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${WASM_ROOT}
      -DCMAKE_BUILD_TYPE=Release
      -DLLVM_TARGETS_TO_BUILD=
      -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly
      -DLLVM_ENABLE_RTTI=ON
  )
")

set(DEPS "${CMAKE_CURRENT_BINARY_DIR}/deps")
set(DEPS_BUILD "${DEPS}/build")
file(MAKE_DIRECTORY ${DEPS} ${DEPS_BUILD})
file(WRITE "${DEPS}/CMakeLists.txt" "${CMAKE_LIST_CONTENT}")

execute_process(WORKING_DIRECTORY "${DEPS_BUILD}" COMMAND ${CMAKE_COMMAND} ..)
execute_process(WORKING_DIRECTORY "${DEPS_BUILD}" COMMAND ${CMAKE_COMMAND} --build .)

set(LLVM_DIR "${WASM_ROOT}/lib/cmake/llvm")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${MONGO_DRIVER_ROOT}")

