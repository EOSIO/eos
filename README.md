# UOS Overview
...


## Starting up the node

Clone the code
```
git clone https://github.com/UOSnetwork/uos
```

Update the submodules
```
cd uos
git submodule update --init --recursive
```

Install the additional libs:
```
Debian/Ubuntu: apt -y install libboost-all-dev
CentOS7: yum -y install boost-devel
```

Compile:
```
./eosio_build.sh
```

Install:
```
./eosio_install.sh
```

Run and stop the node to generate the default config
```
nodeos
Ctrl+C
```

Add the seed nodes to the default config at ~/.local/share/eosio/nodeos/config/config.ini
```
p2p-peer-address = node-1.u.community:9876
p2p-peer-address = node-2.u.community:9876
p2p-peer-address = node-3.u.community:9876
p2p-peer-address = node-4.u.community:9876
p2p-peer-address = node-5.u.community:9876
```
