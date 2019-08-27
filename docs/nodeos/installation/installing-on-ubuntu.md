# Installing on Ubuntu

There are two ways to install EOSIO `nodeos`:
* Automatic installation
* Manual installation

## Automatic Installation

1. Clone the desired release of EOSIO/eos (1.8.x in this example):  
   `$ git clone --single-branch --recursive -b release/1.8.x https://github.com/EOSIO/eos.git`
2. Change to `eos` directory:  
   `$ cd eos`
3. Run build script:  
   `$ ./scripts/eosio_build.sh`
4. Run install script:  
   `$ ./scripts/eosio_install.sh`

## Manual Installation

1. Follow the [Instructions to build EOSIO manually on Ubuntu 18.04](eosio_manual_build_ubuntu_18.04.md).
