# Building via Conan | macOS-10.14

The instructions below can be used to build and test EOS on macOS-10.14.

## Build Steps

**NOTE**: This requires the conan-poc-v2 branch.

**NOTE**: This requires brew to be installed on your system.

```
brew install git cmake python llvm@7 automake autoconf libtool

pip3 install conan

export SDKROOT=$(xcrun --show-sdk-path)

git clone https://github.com/EOSIO/eos.git

cd eos/

git checkout conan-poc-v2

git submodule update --init --recursive

cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -DUSE_CONAN=true -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```
## Test Steps

```
brew install jq python@2 curl

ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

ctest -L nonparallelizable_tests --output-on-failure -T Test
```

## License

[MIT](../LICENSE)

## Important

See [LICENSE](../LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](../IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
