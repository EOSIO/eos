# Mandel

## Repo organization

Current development is on the `main` branch. There aren't any releases yet; they'll eventually be on release branches.

## Supported Operating Systems

To speed up development, we're starting with **Ubuntu 20.04**. We'll support additional operating systems as time and personnel allow. In the mean time, they may break.

## Building

To speed up development and reduce support overhead, we're initially only supporting the following build approach. CMake options not listed here may break. Build scripts may break. Dockerfiles may break.

### Ubuntu 20.04 dependencies

```
apt-get update && apt-get install   \
        binaryen                    \
        build-essential             \
        ccache                      \
        cmake                       \
        curl                        \
        git                         \
        libboost-all-dev            \
        libcurl4-openssl-dev        \
        libgmp-dev                  \
        libssl-dev                  \
        libtinfo5                   \
        libusb-1.0-0-dev            \
        libzstd-dev                 \
        llvm-11-dev                 \
        ninja-build                 \
        pkg-config                  \
        time
```

### Building

```
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc)
```

`make install` isn't supported unless:
* You used cmake's `-DCMAKE_INSTALL_PREFIX=....` option, or
* You're building a docker image

We support the following CMake options:
```
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache    Speed up builds
-DCMAKE_C_COMPILER_LAUNCHER=ccache      Speed up builds
-DCMAKE_BUILD_TYPE=DEBUG                Debug builds
-DDISABLE_WASM_SPEC_TESTS=yes           Speed up builds and skip many tests
-DCMAKE_TARGET_MESSAGES=no              Turn noise down a notch
-DCMAKE_INSTALL_PREFIX=/foo/bar         Where to install to
-GNinja                                 Use ninja instead of make
                                        (faster on high-core-count machines)
```

I highly recommend the ccache options. They don't speed up the first clean build, but they speed up future clean builds after the first build.

### Running tests

```
cd build

# Runs parallelizable tests in parallel. This runs much faster when
# -DDISABLE_WASM_SPEC_TESTS=yes is used.
ctest -j $(nproc) -LE "nonparallelizable_tests|long_running_tests" -E "full-version-label-test|release-build-test|print-build-info-test"

# These tests can't run in parallel.
ctest -L "nonparallelizable_tests"

# These tests can't run in parallel. They also take a long time to run.
ctest -L "long_running_tests"
```
