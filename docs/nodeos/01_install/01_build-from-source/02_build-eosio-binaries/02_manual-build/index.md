# EOSIO Manual Build

[[info]]
| These manual instructions are intended for advanced developers. The [Build Script](../00_build-script.md) should be the preferred method to build EOSIO from source. If the script fails or your platform is not supported, please continue with the instructions below.

The following instructions will build the EOSIO binaries manually by invoking commands on the shell. The following prerequisites must be met before building the EOSIO binaries:

1. [Download EOSIO Source](../../01_download-eosio-source.md).
2. [Install EOSIO Dependencies](00_eosio-dependencies/index.md).

After completing the steps above, perform the instructions below to build the EOSIO software manually:

Create and change to the `build` folder under the `eos` repo folder:
```sh
$ mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build
```

On Linux platforms, use this `cmake` command:
```sh
$ cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=~/wasm-compiler/llvm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBUILD_MONGO_DB_PLUGIN=true ..
```

On MacOS, use this `cmake` command:
```sh
$ cmake -DBINARYEN_BIN=~/binaryen/bin -DWASM_ROOT=/usr/local/wasm -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DBUILD_MONGO_DB_PLUGIN=true ..
```

Then on all platforms except macOS:
```sh
$ make -j$(nproc)
```

For macOS run this:
```sh
$ make -j$(sysctl -n hw.ncpu)
```

Out-of-source builds are also supported. To override clang's default choice in compiler, add these flags to the CMake command:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.
