# Building via Conan | CentOS-7.6

The instructions below can be used to build and test EOS on CentOS-7.6.

## Build Steps

**NOTE**: This requires the conan-poc-v2 branch.

```
yum install -y epel-release

yum --enablerepo=extras install -y centos-release-scl && yum --enablerepo=extras install -y devtoolset-8

yum install -y rh-python36 llvm7.0-devel llvm7.0-static git curl automake

source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable

pip3 install conan

curl -LO https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

chmod +x cmake-3.15.3-Linux-x86_64.sh

./cmake-3.15.3-Linux-x86_64.sh --prefix=/usr/local --include-subdir

git clone https://github.com/EOSIO/eos.git

cd eos/

git checkout conan-poc-v2

git submodule update --init --recursive

/usr/local/cmake-3.15.3-Linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -DLLVM_DIR='/usr/lib64/llvm7.0/lib/cmake/llvm' -DUSE_CONAN=true -Bbuild

cd build/

make -j$(getconf _NPROCESSORS_ONLN)
```
## Test Steps

```
yum install -y jq python python-devel

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test

/usr/local/cmake-3.15.3-Linux-x86_64/bin/ctest -L nonparallelizable_tests --output-on-failure -T Test
```

## License

[MIT](../LICENSE)

## Important

See [LICENSE](../LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](../IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
