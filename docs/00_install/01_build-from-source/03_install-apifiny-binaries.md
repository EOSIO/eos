
## EOSIO install script

For ease of contract development, content can be installed at the `/usr/local` folder using the `apifiny_install.sh` script within the `apifiny/scripts` folder. Adequate permission is required to install on system folders:

```sh
$ cd ~/apifiny/apifiny
$ sudo ./scripts/apifiny_install.sh
```

## EOSIO manual install

In lieu of the `apifiny_install.sh` script, you can install the EOSIO binaries directly by invoking `make install` within the `apifiny/build` folder. Again, adequate permission is required to install on system folders:

```sh
$ cd ~/apifiny/apifiny/build
$ sudo make install
```
