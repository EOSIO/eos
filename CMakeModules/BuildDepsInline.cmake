
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
  set(BOOST_CONF_EXE "bootstrap.bat")
  set(BOOST_BUILD_EXE "b2.exe")
  set(ENV{PATH} "${CMAKE_CURRENT_BINARY_DIR}/deps/build/perl/src/perl/perl/bin/;${CMAKE_CURRENT_BINARY_DIR}/deps/build/nasm/src/nasm/;$ENV{PATH}")
  set(GMP_INSTALL "${CMAKE_CURRENT_BINARY_DIR}/gmp_install")
  set(INTL_INSTALL "${CMAKE_CURRENT_BINARY_DIR}/intl_install")
  set(OPENSSL_INSTALL "${CMAKE_CURRENT_BINARY_DIR}/openssl_install")
  set(MONGO_C_DEPS "openssl")
  set(MONGO_C_CMAKE_EXTRA_ARGS "-DENABLE_EXTRA_ALIGNMENT=OFF -DOPENSSL_ROOT_DIR=${OPENSSL_INSTALL} -DOPENSSL_USE_STATIC_LIBS=ON")
else()
  set(BOOST_CONF_EXE "./bootstrap.sh")
  set(BOOST_BUILD_EXE "./b2")
  set(MONGO_C_DEPS "")
  set(MONGO_C_CMAKE_EXTRA_ARGS "")
endif()

set(CMAKE_LIST_CONTENT "
  cmake_minimum_required(VERSION 3.5)
  include(ExternalProject)

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

    ExternalProject_add(  perl_download
      PREFIX              perl
      SOURCE_DIR          perl/src/perl
      URL                 http://strawberryperl.com/download/5.28.1.1/strawberry-perl-5.28.1.1-64bit.zip
      URL_HASH            SHA1=75c38efbf7b85ef2a83d28e8157a67c3f895050a
      CONFIGURE_COMMAND   \"\"
      BUILD_COMMAND       \"\"
      INSTALL_COMMAND     \"\"
    )

    ExternalProject_add(  nasm_download
      DEPENDS             perl_download
      PREFIX              nasm
      SOURCE_DIR          nasm/src/nasm
      URL                 https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/win64/nasm-2.14.02-win64.zip
      URL_HASH            SHA256=18918AC906E29417B936466E7A2517068206C8DB8C04B9762A5BEFA18BFEA5F0
      CONFIGURE_COMMAND   \"\"
      BUILD_COMMAND       \"\"
      INSTALL_COMMAND     \"\"
    )

    # TODO choose VC-WIN32 or VC-WIN64A
    ExternalProject_add(  openssl
      DEPENDS             nasm_download
      PREFIX              openssl
      GIT_REPOSITORY      https://github.com/openssl/openssl
      GIT_TAG             origin/OpenSSL_1_1_1-stable
      GIT_SHALLOW         1
      GIT_PROGRESS        1
      BUILD_IN_SOURCE     1
      CONFIGURE_COMMAND   perl Configure VC-WIN32 --prefix=${OPENSSL_INSTALL}
      BUILD_COMMAND       nmake
      INSTALL_COMMAND     nmake install_sw
    )
  endif()

  ExternalProject_add(  boost
    PREFIX              boost
    URL                 https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2
    URL_HASH            SHA256=2684c972994ee57fc5632e03bf044746f6eb45d4920c343937a465fd67a5adba
    BUILD_IN_SOURCE     1
    CONFIGURE_COMMAND   ${BOOST_CONF_EXE}
    BUILD_COMMAND       ${BOOST_BUILD_EXE} --prefix=${BOOST_ROOT} -j${NUM_CORES}
    INSTALL_COMMAND     ${BOOST_BUILD_EXE} --prefix=${BOOST_ROOT} install
  )

  ExternalProject_add(  mongo-c-driver
    DEPENDS             ${MONGO_C_DEPS}
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
    ${MONGO_C_CMAKE_EXTRA_ARGS}
  )

  ExternalProject_add(  mongo-cxx-driver
    DEPENDS             mongo-c-driver boost
    PREFIX              monngo-cxx-driver
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

if(WIN32)
  set(ENV{BOOST_ROOT} "${BOOST_ROOT}")
  set(GMP_INCLUDE_DIR "${GMP_INSTALL}/include")
  set(GMP_LIBRARIES "${GMP_INSTALL}/lib/libgmp.a")
  set(Intl_INCLUDE_DIR "${INTL_INSTALL}/include")
  set(Intl_LIBRARY "${INTL_INSTALL}/lib/libintl.lib")
  set(OPENSSL_ROOT_DIR "${OPENSSL_INSTALL}")
  set(OPENSSL_USE_STATIC_LIBS ON)
endif()

