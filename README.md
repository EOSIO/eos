## Building EOSIO

### Setting up the environment.

The environment tools required for building EOSIO can be deduced from the files located at https://github.com/EOSIO/eos/tree/develop/.cicd/platforms/unpinned.

For example, for building it on a Ubuntu 18.04 machine, you can look at its dockerfile image https://github.com/EOSIO/eos/blob/develop/.cicd/platforms/unpinned/ubuntu-18.04-unpinned.dockerfile and copy/paste the commands located there.

### Cloning

Clone as normal, but don't forget to get the submodules as well:

```sh
$ git clone --recursive https://github.com/EOSIO/eos.git -b develop
```

### Running CMake

Run as usual:

```sh
$ cd eos
$ mkdir build
$ cd build
$ cmake ..
$ make -j
```
### Troubleshooting

If, after following the dockerfile commands and running cmake, you get an error that the gcc version installed is lower than what the current cmake supports, such as:

```sh
CMake Error at CMakeLists.txt:42 (message):
  GCC version must be at least 8.0!
```

then you need to either install a new version of the gcc compiler or switch to use the clang compiler, one example of a how to instruct cmake to use clang would be:

```sh
$ cmake -DCMAKE_C_COMPILER=/usr/bin/clang-7 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-7 ..
```