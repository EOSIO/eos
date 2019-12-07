
[[info | Reminder]]
| This section assumes that you have already installed the EOSIO dependencies manually. If not, visit the [Manual Installation of EOSIO Dependencies](00_eosio-dependencies/index.md#manual-installation-of-dependencies) section.

## Instructions

Please follow the steps below to build the EOSIO software manually on your selected OS:

### All Platforms

Create and change to the `build` folder under the `eos` folder:

```sh
$ mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build
```

### MacOS

Use the following command on MacOS to build the EOSIO binaries manually:

```sh
$ bash -c "mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/eosio/eos/.cicd/helpers/clang.make .. && make -j$(getconf _NPROCESSORS_ONLN)"
```

### Amazon Linux or CentOS Linux

Use the following command on Amazon Linux or CentOS Linux to build the EOSIO binaries manually:

```sh
$ bash -c "mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build && export PATH=/usr/lib64/ccache:\$PATH && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/eosio/eos/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache .. && make -j$(nproc)"
```

### Ubuntu Linux

Use the following command on Ubuntu Linux to build the EOSIO binaries manually:

```sh
$ bash -c "mkdir -p ~/eosio/eos/build && cd ~/eosio/eos/build && export PATH=/usr/lib/ccache:\${PATH} && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/eosio/eos/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache .. && make -j$(nproc)"
```

[[info | What's Next?]]
| [Install EOSIO binaries manually](../../03_install-eosio-binaries.md#eosio-manual-install)
