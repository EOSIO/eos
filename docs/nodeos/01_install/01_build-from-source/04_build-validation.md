# EOSIO Build Validation 

Optionally, a set of tests can be run against your build to perform some basic validation.

To run the test suite after building, start `mongod`:

On Linux platforms:
```sh
~/opt/mongodb/bin/mongod -f ~/opt/mongodb/mongod.conf &
```

On MacOS:
```sh
/usr/local/bin/mongod -f /usr/local/etc/mongod.conf &
```

and set the build path to EOSIO_HOME by the following:
```sh
export EOSIO_HOME=~/eosio/eos/build
```

then run `make test` on all platforms:

```sh
cd ~/eosio/eos/build
make test
```

[[info | Recommend]]
| An optional but strongly suggested `make install` step after [building EOSIO from source](index.md) makes local development significantly more friendly.
