## Building And Testing The `blockvault_client_plugin`

### Installing dependencies

Find your corresponding operating system and issue the commands given in their respective CI/CD docker files to install/build the dependecies needed for the `blockvault_client_plugin`:

AmazonLinux2<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/amazon_linux-2-pinned.dockerfile#L44-L52<br>
Centos7.7<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/centos-7.7-pinned.dockerfile#L50-L60<br>
Centos8<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/centos-8-pinned.dockerfile#L47-L58<br>
macOS10.14<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/macos-10.14-pinned.sh#L5<br>
macOS10.15<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/macos-10.15-pinned.sh#L6<br>
Ubuntu16.04<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/ubuntu-16.04-pinned.dockerfile#L45-L54<br>
Ubuntu18.04<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/ubuntu-18.04-pinned.dockerfile#L48-L57<br>
Ubuntu20.04<br>
https://github.com/EOSIO/eos/blob/blockvault/.cicd/platforms/pinned/ubuntu-20.04-pinned.dockerfile#L42-L55<br>

<hr>

### Building the `blockvault` branch

```bash
~ $ git clone --recursive --branch blockvault git@github.com:EOSIO/eos.git blockvault
~ $ cd blockvault
~/blockvault $ mkdir build
~/blockvault $ cd build
cmake ..
make -j10
```

<hr>

### Running the tests to see the `blockvault_client_plugin` in action:
```bash
~/blockvault/build $ #TODO add in the test script
```
