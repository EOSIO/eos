# Build EOSIO Binaries

[[info | Building EOSIO is for Advanced Developers]]
| If you are new to EOSIO, it is suggested that you install the [EOSIO Prebuilt Binaries](../../00_install-prebuilt-binaries.md) instead.

EOSIO can be built on several platforms using different building methods. <!-- TODO: remove? The majority of users may prefer running the docker image.--> Advanced users may opt to build EOSIO from the shell scripts. Node operators or block producers who wish to deploy a public node, may prefer manual methods. The build process writes content in the `~/eosio/eos/build` folder. The program binaries can be found within the `~/eosio/eos/build/programs` sub-folders.

[[info | Installing EOSIO is recommended for All Users]]
| After building EOSIO successfully, it is highly advised to [install the EOSIO binaries](). This copies the EOSIO binaries to a central location, such as `~/eosio/x.y/bin` where `x.y` is the release version.

* [Autobuild Script](00_autobuild-script.md) - Suitable for the majority of developers, this script builds on Mac OS and many flavors of Linux.
<!-- TODO: remove?
* [Docker Compose](01_docker-compose.md) - By far the fastest installation method, you can have a node up and running in a couple minutes. That said, it requires some additional local configuration for development to run smoothly and follow our provided tutorials.
-->
* [Manual Build](02_manual-build.md) - Suitable for those who have environments that may be hostile to the autobuild script or for operators who wish to exercise more control over their build.
