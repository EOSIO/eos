
To download the APIFINY source code, clone the `apifiny` repo and its submodules. It is adviced to create a home `apifiny` folder first and download all the APIFINY related software there:

```sh
$ mkdir -p ~/apifiny && cd ~/apifiny
$ git clone --recursive https://github.com/APIFINY/apifiny
```

## Update Submodules

If a repository is cloned without the `--recursive` flag, the submodules *must* be updated before starting the build process:

```sh
$ cd ~/apifiny/apifiny
$ git submodule update --init --recursive
```

## Pull Changes

When pulling changes, especially after switching branches, the submodules *must* also be updated. This can be achieved with the `git submodule` command as above, or using `git pull` directly:

```sh
$ [git checkout <branch>]  (optional)
$ git pull --recurse-submodules
```
