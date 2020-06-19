---
content_title: Install Prebuilt Binaries
---

[[info | Previous Builds]]
| If you have previously installed EOSIO from source using shell scripts, you must first run the [Uninstall Script](01_build-from-source/01_shell-scripts/05_uninstall-eosio.md) before installing any prebuilt binaries on the same OS.

## Prebuilt Binaries

Prebuilt EOSIO software packages are available for the operating systems below. Find and follow the instructions for your OS:

### Mac OS X:

#### Mac OS X Brew Install
```sh
brew tap eosio/eosio
brew install eosio
```
#### Mac OS X Brew Uninstall
```sh
brew remove eosio
```

### Ubuntu Linux:

#### Ubuntu 18.04 Package Install
```sh
wget https://github.com/eosio/eos/releases/download/v2.1.0-alpha2/eosio_2.1.0-alpha2-ubuntu-18.04_amd64.deb
sudo apt install ./eosio_2.1.0-alpha2-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Package Install
```sh
wget https://github.com/eosio/eos/releases/download/v2.1.0-alpha2/eosio_2.1.0-alpha2-ubuntu-16.04_amd64.deb
sudo apt install ./eosio_2.1.0-alpha2-ubuntu-16.04_amd64.deb
```
#### Ubuntu Package Uninstall
```sh
sudo apt remove eosio
```

### RPM-based (CentOS, Amazon Linux, etc.):

#### RPM Package Install
```sh
wget https://github.com/eosio/eos/releases/download/v2.1.0-alpha2/eosio-2.1.0-alpha2.el7.x86_64.rpm
sudo yum install ./eosio-2.1.0-alpha2.el7.x86_64.rpm
```
#### RPM Package Uninstall
```sh
sudo yum remove eosio
```

## Location of EOSIO binaries

After installing the prebuilt packages, the actual EOSIO binaries will be located under:
* `/usr/opt/eosio/<version-string>/bin` (Linux-based); or
* `/usr/local/Cellar/eosio/<version-string>/bin` (MacOS)

where `version-string` is the EOSIO version that was installed.

Also, soft links for each EOSIO program (`nodeos`, `cleos`, `keosd`, etc.) will be created under `usr/bin` or `usr/local/bin` to allow them to be executed from any directory.

## Previous Versions

To install previous versions of the EOSIO prebuilt binaries:

1. Browse to https://github.com/EOSIO/eos/tags and select the actual version of the EOSIO software you need to install.

2. Scroll down past the `Release Notes` and download the package or archive that you need for your OS.

3. Follow the instructions on the first paragraph above to install the selected prebuilt binaries on the given OS.
