
[[info | Previous Builds]]
| If you have previously installed EOSIO from source using shell scripts, you must first run the [Uninstall Script](01_build-from-source/05_uninstall-eosio.md) before installing any prebuilt binaries on the same OS.

## Prebuilt Binaries

EOSIO prebuilt binary packages are available for specific [Operating Systems](index.md#supported-operating-systems). Find the instructions below for your given OS:

### Mac OS X:

#### Mac OS X Brew Install
```sh
$ brew tap eosio/eosio
$ brew install eosio
```
#### Mac OS X Brew Uninstall
```sh
$ brew remove eosio
```

### Ubuntu Linux:

#### Ubuntu 18.04 Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.0-rc1/eosio_1.8.0-rc1-ubuntu-18.04_amd64.deb
$ sudo apt install ./eosio_1.8.0-rc1-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.0-rc1/eosio_1.8.0-rc1-ubuntu-16.04_amd64.deb
$ sudo apt install ./eosio_1.8.0-rc1-ubuntu-16.04_amd64.deb
```
#### Ubuntu Package Uninstall
```sh
$ sudo apt remove eosio
```

### RPM-based (CentOS, Amazon Linux, etc.):

#### RPM Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.0-rc1/eosio-1.8.0-rc1.el7.x86_64.rpm
$ sudo yum install ./eosio-1.8.0-rc1.el7.x86_64.rpm
```
#### RPM Package Uninstall
```sh
$ sudo yum remove eosio
```

## Location of EOSIO binaries

After installing the prebuilt binaries, the actual EOSIO binaries will be located under the `~/eosio/x.y/bin` folder, where `x.y` is the EOSIO release version that was installed.

## Previous Versions

To install previous versions of the EOSIO prebuilt binaries:

1. Browse to https://github.com/EOSIO/eos/tags and select the actual version of the EOSIO software you need to install.

2. Scroll down past the `Release Notes` and download the package or archive that you need for your OS.

3. Follow the instructions on the first paragraph above to install the selected prebuilt binaries on the given OS.
