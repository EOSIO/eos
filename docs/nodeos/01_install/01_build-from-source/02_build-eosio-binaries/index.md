# Build EOSIO Binaries

[[info | Building EOSIO is for Advanced Developers]]
| If you are new to EOSIO, it is recommended that you install the [EOSIO Prebuilt Binaries](../../00_install-prebuilt-binaries.md) instead of building from source.

EOSIO can be built on several platforms using different building methods. Advanced users may opt to build EOSIO from the shell scripts. Node operators or block producers who wish to deploy a public node, may prefer manual methods.

[[info | Build folder]]
| The build process writes temporary content in the `eos/build` folder. After building, the program binaries can be found at `eos/build/programs`.

After building EOSIO successfully, it is highly recommended to [install the EOSIO binaries](../03_install-eosio-binaries.md). This copies the EOSIO binaries to a central location, such as `~/eosio/x.y/bin`, where `x.y` is the EOSIO release version.

* [Build Script](00_shell-scripts.md#eosio-build.sh) - Suitable for the majority of developers, this script builds on Mac OS and many flavors of Linux.
* [Manual Build](02_manual-build.md) - Suitable for those who have environments that may be hostile to the autobuild script or for operators who wish to exercise more control over their build.
