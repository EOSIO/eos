# Building via Conan | Ubuntu-18.04

The instructions below can be used to build and test EOSIO on Ubuntu-18.04.

## Build Steps

**NOTE**: This requires the conan-poc-v2 branch.

```
apt-get install -y clang llvm-7-dev python3 python3-dev python3-pip git curl automake 

pip3 install conan

curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

chmod +x cmake-3.15.3-Linux-x86_64.sh

./cmake-3.15.3-Linux-x86_64.sh --prefix=/usr/local --include-subdir

git clone https://github.com/EOSIO/eos.git

cd eos/

git checkout conan-poc-v2

git submodule update --init --recursive

/usr/local/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm' -DUSE_CONAN=true -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```
## Test Steps

```
apt-get install -y jq python2.7 python2.7-devel

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -L nonparallelizable_tests --output-on-failure -T Test
```

## License

[MIT](../LICENSE)

## Important

See [LICENSE](../LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](../IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
