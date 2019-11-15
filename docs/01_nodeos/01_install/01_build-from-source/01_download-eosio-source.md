
To download the EOSIO source code, clone the `eos` repo and its submodules. It is adviced to create a home `eosio` folder first and download all the EOSIO related software there:

```sh
$ mkdir -p ~/eosio && cd ~/eosio
$ git clone --recursive https://github.com/EOSIO/eos
```

## Update Submodules

If a repository is cloned without the `--recursive` flag, the submodules *must* be updated before starting the build process:

```sh
$ cd ~/eosio/eos
$ git submodule update --init --recursive
```

## Pull Changes

When pulling changes, especially after switching branches, the submodules *must* also be updated. This can be achieved with the `git submodule` command as above, or using `git pull` directly:

```sh
$ [git checkout <branch>]  (optional)
$ git pull --recurse-submodules
```
