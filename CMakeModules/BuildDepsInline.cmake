
include(ProcessorCount)
ProcessorCount(NUM_CORES)

set(BOOST_ROOT "${CMAKE_CURRENT_BINARY_DIR}/boost_install")
set(MONGO_DRIVER_ROOT "${CMAKE_CURRENT_BINARY_DIR}/mongo_driver_install")
set(WASM_ROOT "${CMAKE_CURRENT_BINARY_DIR}/wasm_install")

set(CMAKE_PARALLEL_OPT "")
if(CMAKE_GENERATOR MATCHES "Unix Makefiles")
  set(CMAKE_PARALLEL_OPT "-j ${NUM_CORES}")
endif()

if(WIN32)
  set(GMP_INSTALL "${CMAKE_CURRENT_BINARY_DIR}/gmp_install")
  set(INTL_INSTALL "${CMAKE_CURRENT_BINARY_DIR}/intl_install")
  set(MONGO_C_CMAKE_EXTRA_ARGS "-DENABLE_EXTRA_ALIGNMENT=OFF")
  set(BOOST_CONF "bootstrap.bat")
  set(BOOST_BUILD "b2.exe")

  # Find OpenSSL
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
    set(OPENSSL_ROOT_DIR "${OPENSSL_INCLUDE_DIR}/..")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
  else()
    message(FATAL_ERROR "OpenSSL not found. Install latest openssl for windows from https://slproweb.com/products/Win32OpenSSL.html . Pick a non-light version for dev files, and install it to 'C:\\OpenSSL' instead of 'C:\\Program Files\\OpenSSL-Win64'")
  endif()
else()
  set(MONGO_C_CMAKE_EXTRA_ARGS "")
  set(BOOST_CONF "./bootstrap.sh")
  set(BOOST_BUILD "./b2")
endif()
set(BOOST_BUILD "${BOOST_BUILD} variant=release link=static --prefix=${BOOST_ROOT} -j${NUM_CORES}")

set(CMAKE_LIST_CONTENT "
  cmake_minimum_required(VERSION 3.5)
  include(ExternalProject)

  ExternalProject_add(  boost
    PREFIX              boost
    URL                 https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2
    URL_HASH            SHA256=2684c972994ee57fc5632e03bf044746f6eb45d4920c343937a465fd67a5adba
    BUILD_IN_SOURCE     1
    CONFIGURE_COMMAND   ${BOOST_CONF}
    BUILD_COMMAND       ${BOOST_BUILD}
    INSTALL_COMMAND     ${BOOST_BUILD} install
  )

  ExternalProject_add(  mongo-c-driver
    PREFIX              mongo-c-driver
    GIT_REPOSITORY      https://github.com/mongodb/mongo-c-driver
    GIT_TAG             1.13.1
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    BUILD_COMMAND       \"${CMAKE_COMMAND}\" --build . -- ${CMAKE_PARALLEL_OPT}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${MONGO_DRIVER_ROOT}
      -DCMAKE_BUILD_TYPE=Release
      -DENABLE_STATIC=ON
      -DENABLE_SSL=OPENSSL
      -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF
      -DENABLE_TESTS=OFF
      -DENABLE_EXAMPLES=OFF
      ${MONGO_C_CMAKE_EXTRA_ARGS}
  )

  ExternalProject_add(  mongo-cxx-driver
    DEPENDS             mongo-c-driver boost
    PREFIX              mongo-cxx-driver
    GIT_REPOSITORY      https://github.com/mongodb/mongo-cxx-driver
    GIT_TAG             origin/releases/stable
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    BUILD_COMMAND       \"${CMAKE_COMMAND}\" --build . -- ${CMAKE_PARALLEL_OPT}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${MONGO_DRIVER_ROOT}
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_SHARED_LIBS=OFF
      -DBOOST_ROOT=${BOOST_ROOT}
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
    BUILD_COMMAND       \"${CMAKE_COMMAND}\" --build . -- ${CMAKE_PARALLEL_OPT}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${WASM_ROOT}
      -DCMAKE_BUILD_TYPE=Release
      -DLLVM_TARGETS_TO_BUILD=
      -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly
      -DLLVM_ENABLE_RTTI=ON
      -DLLVM_BUILD_EXAMPLES=OFF
      -DLLVM_BUILD_TESTS=OFF
  )

  if(WIN32)
    ExternalProject_add(  gmp_download
      PREFIX              gmp
      SOURCE_DIR          gmp/src/gmp
      URL                 https://sourceforge.net/projects/mingw/files/MinGW/Base/gmp/gmp-6.1.2/gmp-6.1.2-2-mingw32-dev.tar.xz/download
      URL_HASH            SHA256=2EE1F5553B3A16295E5BDB13D7325EDC03C69046B51332DE9C2D1C60B1F04F1A
      CONFIGURE_COMMAND   \"\"
      BUILD_COMMAND       \"\"
      INSTALL_COMMAND     \"${CMAKE_COMMAND}\" -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/deps/build/gmp/src/gmp/ ${GMP_INSTALL}
    )

    ExternalProject_add(  intl_download
      PREFIX              intl
      SOURCE_DIR          intl/src/intl
      URL                 http://gnuwin32.sourceforge.net/downlinks/libintl-lib-zip.php
      URL_HASH            SHA256=5C0588041ED0188FA78067A8A9F66E0531B51C6198F37794B94699CA71FFABD7
      CONFIGURE_COMMAND   \"\"
      BUILD_COMMAND       \"\"
      INSTALL_COMMAND     \"${CMAKE_COMMAND}\" -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/deps/build/intl/src/intl/ ${INTL_INSTALL}
    )
  endif()
")

set(DEPS "${CMAKE_CURRENT_BINARY_DIR}/deps")
set(DEPS_BUILD "${DEPS}/build")
file(MAKE_DIRECTORY ${DEPS} ${DEPS_BUILD})
file(WRITE "${DEPS}/CMakeLists.txt" "${CMAKE_LIST_CONTENT}")

# unset DESTDIR set by snapcraft
set(DESTDIR_ORIG "$ENV{DESTDIR}")
set(ENV{DESTDIR} "")

execute_process(WORKING_DIRECTORY "${DEPS_BUILD}" COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} ..)
execute_process(WORKING_DIRECTORY "${DEPS_BUILD}" COMMAND ${CMAKE_COMMAND} --build .)

# set DESTDIR back
set(ENV{DESTDIR} "${DESTDIR_ORIG}")

set(LLVM_DIR "${WASM_ROOT}/lib/cmake/llvm")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${MONGO_DRIVER_ROOT}")

if(WIN32)
  set(ENV{BOOST_ROOT} "${BOOST_ROOT}")
  set(GMP_INCLUDE_DIR "${GMP_INSTALL}/include")
  set(GMP_LIBRARIES "${GMP_INSTALL}/lib/libgmp.a")
  set(Intl_INCLUDE_DIR "${INTL_INSTALL}/include")
  set(Intl_LIBRARY "${INTL_INSTALL}/lib/libintl.lib")
endif()

