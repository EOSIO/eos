
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
export EOSIO_HOME=~/apifiny/apifiny/build
```

then run `make test` on all platforms:

```sh
cd ~/apifiny/apifiny/build
make test
```

[[info | Recommend]]
| It is strongly suggested to [Install the EOSIO binaries](03_install-apifiny-binaries.md) after building EOSIO from source as it makes local development significantly more friendly.
