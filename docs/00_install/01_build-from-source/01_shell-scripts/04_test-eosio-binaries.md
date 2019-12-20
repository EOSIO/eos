---
content_title: Test EOSIO Binaries
---

Optionally, a set of tests can be run against your build to perform some basic validation of the EOSIO software installation.

To run the test suite after building, start `mongod`:

On Linux platforms:
```sh
~/opt/mongodb/bin/mongod -f ~/opt/mongodb/mongod.conf &
```

On MacOS:
```sh
/usr/local/bin/mongod -f /usr/local/etc/mongod.conf &
```

then set the build path to EOSIO_HOME:
```sh
export EOSIO_HOME=~/eosio/eos/build
```

then run `make test` on all platforms:

```sh
cd ~/eosio/eos/build
make test
```

[[info | What's Next?]]
| Configure and use [Nodeos](../../../01_nodeos/index.md).
