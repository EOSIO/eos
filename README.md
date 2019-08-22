
# CAN - Cryptobadge Alliance Network

CAN is a merit-based identity network, powered by CryptoBadge, a blockchain-based certificate system. CryptoBadgelets users identify themselves through expertise, achievements, and contributions. It captures a variety of certifiable values and lets you build your personal brands. CryptoBadge is a universal, eternal, and verifiable acknowledgement system shared across communities. CryptoBadge divides badge winner’s merits into tokenizable assets and provides the users with diverse opportunities to gain reputation, power, and economic benefit


---

**If you used our build scripts to install eosio, [please be sure to uninstall](#build-script-uninstall) before using our packages.**

---

#### Mac OS X Brew Install
```sh
$ brew tap eosio/eosio
$ brew install eosio
```
#### Mac OS X Brew Uninstall
```sh
$ brew remove eosio
```

#### Ubuntu 18.04 Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.1/eosio_1.8.1-1-ubuntu-18.04_amd64.deb
$ sudo apt install ./eosio_1.8.1-1-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.1/eosio_1.8.1-1-ubuntu-16.04_amd64.deb
$ sudo apt install ./eosio_1.8.1-1-ubuntu-16.04_amd64.deb
```
#### Ubuntu Package Uninstall
```sh
$ sudo apt remove eosio
```
#### Centos RPM Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.8.1/eosio-1.8.1-1.el7.x86_64.rpm
$ sudo yum install ./eosio-1.8.1-1.el7.x86_64.rpm
```
#### Centos RPM Package Uninstall
```sh
$ sudo yum remove eosio
```

#### Build Script Uninstall

If you have previously installed EOSIO using build scripts, you can execute `eosio_uninstall.sh` to uninstall.
- Passing `-y` will answer yes to all prompts (does not remove data directories)
- Passing `-f` will remove data directories (be very careful with this)
- Passing in `-i` allows you to specify where your eosio installation is located

## Supported Operating Systems
CAN currently supports the following operating systems:  
1. Amazon Linux 2
2. CentOS 7
3. Ubuntu 16.04
4. Ubuntu 18.04
5. MacOS 10.14 (Mojave)
