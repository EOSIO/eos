
[[info | Reminder]]
| This section assumes that you have already installed the APIFINY dependencies manually. If not, visit the [Manual Installation of APIFINY Dependencies](00_apifiny-dependencies/index.md#manual-installation-of-dependencies) section.

## Instructions

Please follow the steps below to build the APIFINY software manually on your selected OS:

### All Platforms

Create and change to the `build` folder under the `apifiny` folder:

```sh
$ mkdir -p ~/apifiny/apifiny/build && cd ~/apifiny/apifiny/build
```

### MacOS

Use the following command on MacOS to build the APIFINY binaries manually:

```sh
$ bash -c "mkdir -p ~/apifiny/apifiny/build && cd ~/apifiny/apifiny/build && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/apifiny/apifiny/.cicd/helpers/clang.make .. && make -j$(getconf _NPROCESSORS_ONLN)"
```

### Amazon Linux or CentOS Linux

Use the following command on Amazon Linux or CentOS Linux to build the APIFINY binaries manually:

```sh
$ bash -c "mkdir -p ~/apifiny/apifiny/build && cd ~/apifiny/apifiny/build && export PATH=/usr/lib64/ccache:\$PATH && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/apifiny/apifiny/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache .. && make -j$(nproc)"
```

### Ubuntu Linux

Use the following command on Ubuntu Linux to build the APIFINY binaries manually:

```sh
$ bash -c "mkdir -p ~/apifiny/apifiny/build && cd ~/apifiny/apifiny/build && export PATH=/usr/lib/ccache:\${PATH} && cmake -DCMAKE_BUILD_TYPE='Release' -DBUILD_MONGO_DB_PLUGIN=true -DCMAKE_TOOLCHAIN_FILE=~/apifiny/apifiny/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache .. && make -j$(nproc)"
```

[[info | What's Next?]]
| [Install APIFINY binaries manually](../../03_install-apifiny-binaries.md#apifiny-manual-install)
