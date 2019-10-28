# Build EOSIO Binaries

[[info | Building EOSIO is Recommended for Advanced Developers]]
| If you are new to EOSIO, it is suggested that you use the [Docker Quickstart Guide](../../02_docker-quickstart.md) instead, or [Install EOSIO Prebuilt Binaries](../../00_install-prebuilt-binaries.md) directly.

EOSIO can be built on several platforms using different building methods. The majority of users may prefer running the autobuild script or docker image, while more advanced users or block producers wishing to deploy a public node, may prefer manual methods. The build process writes content in the `eos/build` folder. The executables can be found in subfolders within the `eos/build/programs` folder.

* [Autobuild Script](00_autobuild-script.md) - Suitable for the majority of developers, this script builds on Mac OS and many flavors of Linux.

* [Docker Compose](01_docker-compose.md) - By far the fastest installation method, you can have a node up and running in a couple minutes. That said, it requires some additional local configuration for development to run smoothly and follow our provided tutorials.

* [Manual Build](02_manual-build.md) - Suitable for those who have environments that may be hostile to the autobuild script or for operators who wish to exercise more control over their build.
