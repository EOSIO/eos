---
content_title: Build EOSIO from Source
---

[[info | Building EOSIO is for Advanced Developers]]
| If you are new to EOSIO, it is recommended that you install the [EOSIO Prebuilt Binaries](../00_install-prebuilt-binaries.md) instead of building from source.

EOSIO can be built on several platforms using different build methods. Advanced users may opt to build EOSIO using our shell scripts. Node operators or block producers who wish to deploy a public node, may prefer our manual build instructions.

* [Shell Scripts](01_shell-scripts/index.md) - Suitable for the majority of developers, these scripts build on Mac OS and many flavors of Linux.
* [Manual Build](02_manual-build/index.md) - Suitable for those platforms that may be hostile to the shell scripts or for operators who need more control over their builds.

[[info | EOSIO Installation Recommended]]
| After building EOSIO successfully, it is highly recommended to install the EOSIO binaries from their default build directory. This copies the EOSIO binaries to a central location, such as `/usr/local/bin`, or `~/eosio/x.y/bin`, where `x.y` is the EOSIO release version.
