## Building And Testing The `blockvault_client_plugin`
*this guide assumes the user is using macOS and has the appropriate dependencies installed for building `EOSIO/eos`<br>
*to build and test the `blockvault_client_plugin` as it stands a user needs to have the dependencies `libpq`, `libpqxx`, and `docker`:

<hr>

### Building the `blockvault` branch:

Install dependencies `libpq` and `libpqxx`:
```bash
brew install libpq libpqxx
```

Install `docker`:
```bash
brew cask install docker
```

Build the `blockvault` branch:
```bash
~ $ git clone --recursive --branch blockvault git@github.com:EOSIO/eos.git blockvault
~ $ cd blockvault
~/blockvault $ mkdir build
~/blockvault $ cd build
cmake ..
make -j10
```

<hr>

### Running the `nodeos_run_test.py` to see the `blockvault_client_plugin` in action:
```bash
~/blockvault $ cd build
~/blockvault/build $ #TODO add in the test script
```
