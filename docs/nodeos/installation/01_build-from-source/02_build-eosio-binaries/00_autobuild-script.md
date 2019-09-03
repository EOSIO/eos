# EOSIO Autobuild Script

An automated build script is provided to install all dependencies and build EOSIO. The script supports the following operating systems:

* Amazon Linux 2
* CentOS 7
* Ubuntu 16.04
* Ubuntu 18.04
* MacOS 10.14 (Mojave)

Run the build script from the `eos` folder:

```sh
$ cd ~/eosio/eos
$ ./scripts/eosio_build.sh
```

After the script has completed, install EOSIO:

```sh
$ cd ~/eosio/eos
$ sudo ./scripts/eosio_install.sh
```
