# Install EOSIO Prebuilt Binaries

EOSIO prebuilt binaries are available for the following operating systems:  
1. Amazon Linux 2
2. CentOS 7
3. Ubuntu 16.04
4. Ubuntu 18.04
5. MacOS 10.14 (Mojave)

It may be possible to install EOSIO on other Unix-based operating systems, but this is not officially supported. 

## Build Script Uninstall

If you have previously installed EOSIO from source using build scripts, you must first uninstall EOSIO before installing any prebuilt binaries on the same OS:

```sh
$ cd ~/eosio/eos/scripts
$ ./eosio_uninstall.sh [-y] [-f] [-i DIR]
```

- Passing `-y` will answer yes to all prompts (does not remove data directories).
- Passing `-f` will remove data directories (be very careful with this).
- Passing `-i` allows you to specify where your eosio installation is located.

## Install/Uninstall Prebuilt Binaries

If you have previously installed EOSIO using prebuilt binaries, you must first uninstall EOSIO before reinstalling any prebuilt binaries on the same OS.

### Mac OS X:

#### Mac OS X Brew Uninstall
```sh
$ brew remove eosio
```
#### Mac OS X Brew Install
```sh
$ brew tap eosio/eosio
$ brew install eosio
```

### Ubuntu Linux:

#### Ubuntu Package Uninstall
```sh
$ sudo apt remove eosio
```
#### Ubuntu 18.04 Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.2/eosio_1.8.2-1-ubuntu-18.04_amd64.deb
$ sudo apt install ./eosio_1.8.2-1-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.2/eosio_1.8.2-1-ubuntu-16.04_amd64.deb
$ sudo apt install ./eosio_1.8.2-1-ubuntu-16.04_amd64.deb
```

### RPM-based (CentOS, Amazon Linux, etc.):

#### RPM Package Uninstall
```sh
$ sudo yum remove eosio
```
#### RPM Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.2/eosio-1.8.2-1.el7.x86_64.rpm
$ sudo yum install ./eosio-1.8.2-1.el7.x86_64.rpm
```

## Location of EOSIO binaries

After installing the prebuilt binaries, the actual EOSIO binaries will be located under the `eosio/x.y.z/bin` folder, where `x.y.z` is the EOSIO release version that was installed.

## Previous Versions

To install previous versions of the EOSIO prebuilt binaries:

1. Browse to https://github.com/EOSIO/eos/tags and select the actual EOSIO version that you need to install.

2. Scroll down past the `Release Notes` and download the package or archive that you need for your OS.

3. Follow the instructions above to [Install/Uninstall the prebuilt binaries](#install/uninstall-prebuilt-binaries) for that OS.
