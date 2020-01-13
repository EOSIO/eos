
# EOSIO - The Most Powerful Infrastructure for Decentralized Applications

[![Build status](https://badge.buildkite.com/370fe5c79410f7d695e4e34c500b4e86e3ac021c6b1f739e20.svg?branch=master)](https://buildkite.com/EOSIO/eosio)
[![Release](https://img.shields.io/github/v/release/eosio/eos.svg)](https://github.com/eosio/eos/releases)

Welcome to the EOSIO source code repository! This software enables businesses to rapidly build and deploy high-performance and high-security blockchain-based applications.

Some of the groundbreaking features of EOSIO include:

1. Free Rate Limited Transactions
1. Low Latency Block confirmation (0.5 seconds)
1. Low-overhead Byzantine Fault Tolerant Finality
1. Designed for optional high-overhead, low-latency BFT finality
1. Smart contract platform powered by WebAssembly
1. Designed for Sparse Header Light Client Validation
1. Scheduled Recurring Transactions
1. Time Delay Security
1. Hierarchical Role Based Permissions
1. Support for Biometric Hardware Secured Keys (e.g. Apple Secure Enclave)
1. Designed for Parallel Execution of Context Free Validation Logic
1. Designed for Inter Blockchain Communication

## License

EOSIO is released under the open source MIT license and is offered “AS IS” without warranty of any kind, express or implied. Any security provided by the EOSIO software depends in part on how it is used, configured, and deployed. EOSIO is built upon many third-party libraries such as WABT (Apache License) and WAVM (BSD 3-clause) which are also provided “AS IS” without warranty of any kind. Without limiting the generality of the foregoing, Block.one makes no representation or guarantee that EOSIO or any third-party libraries will perform as intended or will be free of errors, bugs or faulty code. Both may fail in large or small ways that could completely or partially limit functionality or compromise computer systems. If you use or implement EOSIO, you do so at your own risk. In no event will Block.one be liable to any party for any damages whatsoever, even if it had been advised of the possibility of damage.  

## Disclaimer

Block.one is neither launching nor operating any initial public blockchains based upon the EOSIO software. This release refers only to version 1.0 of our open source software. We caution those who wish to use blockchains built on EOSIO to carefully vet the companies and organizations launching blockchains based on EOSIO before disclosing any private keys to their derivative software.

## Testnets

There is no public testnet running currently.

## Supported Operating Systems

EOSIO currently supports the following operating systems:

1. Amazon Linux 2
2. CentOS 7
3. Ubuntu 16.04
4. Ubuntu 18.04
5. MacOS 10.14 (Mojave)

---

**Note: It may be possible to install EOSIO on other Unix-based operating systems. This is not officially supported, however.**

---

## Software Installation

There are currently two methods to install the EOSIO software:

* Install from [Prebuilt Binaries](#prebuilt-binaries)
* Build/Install from [Shell Scripts](#shell-scripts)

---

**Note: If you used our shell scripts to build/install EOSIO, please run the [Uninstall Script](#uninstall-script) before using our prebuilt binary packages.**

---

Installing EOSIO from prebuilt binaries is recommended for most users. Building and installing from source code is available for advanced users.

## Prebuilt Binaries

Prebuilt software packages are available for the EOSIO [supported operating systems](#supported-operating-systems) above. Find the instructions below for your OS.

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

## Shell Scripts

Shell scripts are provided in the EOSIO repository for building, installing, and uninstalling the EOSIO binaries and dependencies from source.

---

**Note: If you used our shell scripts and the EOSIO installation failed, please run the [Uninstall Script](#uninstall-script) before building EOSIO.**

---

Check the **Build from Source** section in the documentation for more information. Below are the basic shell scripts to get started quickly.

### Build Script
To build the EOSIO binaries and install dependencies, run:
```sh
git clone --recursive https://github.com/eosio/eos
cd eos
./scripts/eosio_build.sh
```

### Install Script
To install the EOSIO built binaries, run:
```sh
./scripts/eosio_install.sh
```

### Uninstall Script
To uninstall the EOSIO built/installed binaries and dependencies, run:
```sh
./scripts/eosio_uninstall.sh
```

## Resources
1. [Website](https://eos.io)
1. [Blog](https://medium.com/eosio)
1. [Developer Portal](https://developers.eos.io)
1. [StackExchange for Q&A](https://eosio.stackexchange.com/)
1. [Community Telegram Group](https://t.me/EOSProject)
1. [Developer Telegram Group](https://t.me/joinchat/EaEnSUPktgfoI-XPfMYtcQ)
1. [White Paper](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md)
1. [Roadmap](https://github.com/EOSIO/Documentation/blob/master/Roadmap.md)

<a name="gettingstarted"></a>
## Getting Started
Instructions detailing the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain can be found in [Getting Started](https://developers.eos.io/eosio-home/docs) on the [EOSIO Developer Portal](https://developers.eos.io).

## Contributing

[Contributing Guide](./CONTRIBUTING.md)

[Code of Conduct](./CONTRIBUTING.md#conduct)

## License

[MIT](./LICENSE)

## Important

See [LICENSE](./LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](./IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
