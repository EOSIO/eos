---
content_title: Build EOSIO Binaries
---

[[info | Shell Scripts]]
| The build script is one of various automated shell scripts provided in the EOSIO repository for building, installing, and optionally uninstalling the EOSIO software and its dependencies. They are available in the `eos/scripts` folder.

The build script first installs all dependencies and then builds EOSIO. The script supports these [Operating Systems](../../index.md#supported-operating-systems). To run it, first change to the `~/eosio/eos` folder, then launch the script:

```sh
$ cd ~/eosio/eos
$ ./scripts/eosio_build.sh
```

The build process writes temporary content to the `eos/build` folder. After building, the program binaries can be found at `eos/build/programs`.

[[info | What's Next?]]
| [Installing EOSIO](03_install-eosio-binaries.md) is strongly recommended after building from source as it makes local development significantly more friendly.
