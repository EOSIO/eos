## Building And Testing The `blockvault_client_plugin`
*this guide assumes the user is using macOS and has the appropriate dependencies installed for building `EOSIO/eos`<br>
*to build and test the `blockvault_client_plugin` as it stands a user needs to have the dependencies `libpq`, `libpqxx`, and `docker`:

<hr>

### Building the `blockvault` branch:

Install dependencies `libpq` and `libpqxx`:
```bash
brew install libpq libpqxx
```

Install docker:
```bash
brew cask install `docker`
```

Build the `blockvault` branch:
```bash
~ $ git clone --recursive --branch blockvault git@github.com:EOSIO/eos.git blockvault
~ $ cd blockvault
~/blockvault $ mkdir build
~/blockvault $ cd build

# Caveat Alert! macOS will default to using its system version of `libpq`. To use the `libpq` installed by brew set the environment variable like so:
~/blockvault $ PKG_CONFIG_PATH=/usr/local/opt/libpq/lib/pkgconfig

cmake ..
make -j10
```

<hr>

### Running the `nodeos_run_test.py` to see the `blockvault_client_plugin` in action:
```bash
~/blockvault $ cd build
~/blockvault/build $ docker run -p 5432:5432 -e POSTGRES_PASSWORD=password -d postgres
~/blockvault/build $ ./tests/nodeos_run_test.py --keep-logs --clean-run -v

# Note: you may need to restart docker upon each subsequent run to clear the POSTGRES databse
# Also Note: you can delete the docker container by running `docker ps` and delete the corresponding docker CONTAINER ID like so: `docker rm -f <CONTAINER ID>`

# Afterwhich you can now tail the stderr log of the `nodeos` node `node_00` (note: that file is given a unique ID on each subsequent run):
~/blockvault/build tail -f ./var/lib/node_00/stderr.2020_11_17_00_58_06.txt | grep blockvault
```
