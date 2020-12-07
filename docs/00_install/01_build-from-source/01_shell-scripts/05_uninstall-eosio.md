---
content_title: Uninstall EOSIO
---

If you have previously built EOSIO from source and now wish to install the prebuilt binaries, or to build from source again, it is recommended to run the `eosio_uninstall.sh` script within the `eos/scripts` folder:

```sh
cd ~/eosio/eos
./scripts/eosio_uninstall.sh
```

[[info | Uninstall Dependencies]]
| The uninstall script will also prompt the user to uninstall EOSIO dependencies. This is recommended if installing prebuilt EOSIO binaries or building EOSIO for the first time.
