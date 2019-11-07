# EOSIO Software Dependencies

The EOSIO software requires specific software dependencies to build the EOSIO binaries. These dependencies can be built from source or installed from binaries directly. Dependencies can be pinned to a specific version release or unpinned to the current version, usually the latest. The main EOSIO dependencies hosted outside the EOSIO repos are:

* Clang - the C++17 compliant compiler used by EOSIO
* CMake - the build system used by EOSIO
* Boost - the C++ Boost library used by EOSIO
* OpenSSL - the secure communications (and crypto) library
* LLVM - the LLVM compiler/toolchain infrastructure

Other dependencies are either inside the EOSIO repo, such as the `secp256k1` elliptic curve DSA library, or they are otherwise used for testing or housekeeping purposes, such as:

* automake, autoconf, autotools
* doxygen, graphviz
* python2, python3
* bzip2, zlib
* etc.

## Pinned Dependencies

To guarantee interoperability across different EOSIO software releases, developers may opt to reproduce the exact "pinned" dependency binaries used in-house. Block producers may want to install and run the EOSIO software built with these pinned dependencies for portability and stability reasons. Pinned dependencies are usually built from source.

## Unpinned Dependencies

Regular users or application developers may prefer installing unpinned versions of the EOSIO dependencies. These correspond to the latest or otherwise stable versions of the dependencies. The main advantage of unpinned dependencies is reduced installation times and perhaps better performance. Pinned dependencies are typically installed from binaries.

## Automatic Installation of Dependencies

EOSIO dependencies can be built or installed automatically from the [Build Script](../../00_build-script.md) when [building EOSIO from Source](../../../index.md). To build the pinned dependencies, the optional `-P` parameter can be specified when invoking the script. Otherwise, the unpinned dependencies will be installed instead, with the exception of `Boost` and `Cmake` which are always pinned:

```sh
$ cd ~/eosio/eos
$ ./scripts/eosio_build.sh [-P]
```

## Manual Installation of Dependencies

EOSIO dependencies can also be built and installed manually by reproducing the same commands invoked by the [Build Script](../../00_build-script). The actual commands can be generated from the script directly by exporting specific environment variables and CLI parameters to the script when invoked:

```sh
$ cd ~/eosio/eos
$ export VERBOSE=true && export DRYRUN=true && ./scripts/eosio_build.sh -y [-P]
```

Below are instructions to install the EOSIO dependencies manually on various platforms:

* [MacOS Mojave](00_macos-mojave.md)
* [Amazon Linux](01_amazon-linux.md)
* [CentOS Linux](02_centos-linux.md)
* [Ubuntu Linux](03_ubuntu-linux.md)

[[info]]
| [Automatic installation of dependencies](#automatic-installation-of-dependencies) should be preferred over manual installation if possible.
