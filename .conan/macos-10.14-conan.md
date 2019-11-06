# Building via Conan | macOS-10.14

The instructions below can be used to build and test EOSIO on macOS-10.14.

**NOTE**: This requires the conan-poc-v2 branch.

**NOTE**: This requires Homebrew to be installed on your system.

## Environment Steps

```
brew install git cmake python automake autoconf libtool jq python@2 curl

pip3 install conan
```

## Build Steps


```
export SDKROOT=$(xcrun --show-sdk-path)

git clone https://github.com/EOSIO/eos.git

cd eos/

git checkout conan-poc-v3

git submodule update --init --recursive

cmake -DCMAKE_BUILD_TYPE='Release' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -DUSE_CONAN=true -Bbuild

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
