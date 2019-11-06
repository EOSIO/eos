# Building via Conan | Ubuntu-18.04

The instructions below can be used to build and test EOSIO on Ubuntu-18.04.

**NOTE**: This requires the conan-poc-v2 branch.

## Environment Steps

```
apt-get install -y clang-7 llvm-7-dev python3 python3-dev python3-pip git curl automake jq python2.7 python2.7-dev

pip3 install conan

curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

mkdir -p /usr/local/cmake

chmod +x cmake-3.15.3-Linux-x86_64.sh

./cmake-3.15.3-Linux-x86_64.sh --prefix=/usr/local/cmake --skip-license
```

## Build Steps

```
git clone https://github.com/EOSIO/eos.git

cd eos/

git checkout conan-poc-v3

git submodule update --init --recursive

cmake -DCMAKE_BUILD_TYPE='Release' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -DCMAKE_CXX_COMPILER='clang++-7' -DCMAKE_C_COMPILER='clang-7' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm' -DUSE_CONAN=true -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```

## Test Steps

```
ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

ctest -L nonparallelizable_tests --output-on-failure -T Test
```

## License

[MIT](../LICENSE)

## Important

See [LICENSE](../LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](../IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
