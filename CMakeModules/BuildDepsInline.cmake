
include(ProcessorCount)
ProcessorCount(NUM_CORES)

set(BOOST_ROOT "${CMAKE_CURRENT_BINARY_DIR}/boost_install")
set(MONGO_DRIVER_ROOT "${CMAKE_CURRENT_BINARY_DIR}/mongo_driver_install")
set(LLVM_INSTALL "${CMAKE_CURRENT_BINARY_DIR}/llvm_install")

set(CMAKE_PARALLEL_OPT "")
if(CMAKE_GENERATOR MATCHES "Unix Makefiles")
  set(CMAKE_PARALLEL_OPT "-j ${NUM_CORES}")
endif()

set(BOOST_CONF "./bootstrap.sh")
set(BOOST_BUILD "./b2 variant=release link=static --prefix=${BOOST_ROOT} -j${NUM_CORES}")

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
  )

  ExternalProject_add(  llvm
    PREFIX              llvm
    GIT_REPOSITORY      https://github.com/llvm-mirror/llvm
    GIT_TAG             release_40
    GIT_SHALLOW         1
    GIT_PROGRESS        1
    BUILD_COMMAND       \"${CMAKE_COMMAND}\" --build . -- ${CMAKE_PARALLEL_OPT}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${LLVM_INSTALL}
      -DCMAKE_BUILD_TYPE=Release
      -DLLVM_TARGETS_TO_BUILD=
      -DLLVM_ENABLE_RTTI=ON
      -DLLVM_BUILD_EXAMPLES=OFF
      -DLLVM_BUILD_TESTS=OFF
  )
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

set(LLVM_DIR "${LLVM_INSTALL}/lib/cmake/llvm")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${MONGO_DRIVER_ROOT}")

