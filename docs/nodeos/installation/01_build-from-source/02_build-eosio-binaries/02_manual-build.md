# Manual Build

To build `eos` manually, use the following steps to create a `build` folder within your `eos` folder and then perform the build. The steps below assume the `eos` repository was cloned into your home eosio folder (i.e., `~/eosio`). It is also assumed that the necessary dependencies have been installed. See [EOSIO Software Dependencies](../../dependencies/index.md).

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
