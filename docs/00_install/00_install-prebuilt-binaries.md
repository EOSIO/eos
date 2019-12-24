
[[info | Previous Builds]]
| If you have previously installed APIFINY from source using shell scripts, you must first run the [Uninstall Script](01_build-from-source/05_uninstall-apifiny.md) before installing any prebuilt binaries on the same OS.

## Prebuilt Binaries

APIFINY prebuilt binary packages are available for specific [Operating Systems](index.md#supported-operating-systems). Find the instructions below for your given OS:

### Mac OS X:

#### Mac OS X Brew Install
```sh
$ brew tap apifiny/apifiny
$ brew install apifiny
```
#### Mac OS X Brew Uninstall
```sh
$ brew remove apifiny
```

### Ubuntu Linux:

#### Ubuntu 18.04 Package Install
```sh
$ wget https://github.com/apifiny/apifiny/releases/download/v1.8.0-rc1/apifiny_1.8.0-rc1-ubuntu-18.04_amd64.deb
$ sudo apt install ./apifiny_1.8.0-rc1-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Package Install
```sh
$ wget https://github.com/apifiny/apifiny/releases/download/v1.8.0-rc1/apifiny_1.8.0-rc1-ubuntu-16.04_amd64.deb
$ sudo apt install ./apifiny_1.8.0-rc1-ubuntu-16.04_amd64.deb
```
#### Ubuntu Package Uninstall
```sh
$ sudo apt remove apifiny
```

### RPM-based (CentOS, Amazon Linux, etc.):

#### RPM Package Install
```sh
$ wget https://github.com/apifiny/apifiny/releases/download/v1.8.0-rc1/apifiny-1.8.0-rc1.el7.x86_64.rpm
$ sudo yum install ./apifiny-1.8.0-rc1.el7.x86_64.rpm
```
#### RPM Package Uninstall
```sh
$ sudo yum remove apifiny
```

## Location of APIFINY binaries

After installing the prebuilt binaries, the actual APIFINY binaries will be located under the `~/apifiny/x.y/bin` folder, where `x.y` is the APIFINY release version that was installed.

## Previous Versions

To install previous versions of the APIFINY prebuilt binaries:

1. Browse to https://github.com/APIFINY/apifiny/tags and select the actual version of the APIFINY software you need to install.

2. Scroll down past the `Release Notes` and download the package or archive that you need for your OS.

3. Follow the instructions on the first paragraph above to install the selected prebuilt binaries on the given OS.
