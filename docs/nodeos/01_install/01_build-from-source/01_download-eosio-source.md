# Download EOSIO Source

To download the EOSIO source code, clone the `eos` repo and its submodules. It is adviced to create a home `eosio` folder first and download all the EOSIO related software there:

```sh
$ mkdir -p ~/eosio && cd ~/eosio
$ git clone https://github.com/EOSIO/eos --recursive
```

If a repository is cloned without the `--recursive` flag, the submodules *must* be updated before starting the build process:

```sh
$ cd ~/eosio/eos
$ git submodule update --init --recursive
```
