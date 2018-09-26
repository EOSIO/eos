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

Add the seed nodes to the default config.ini
```
p2p-peer-address = 159.69.157.38
p2p-peer-address = 159.69.157.39
p2p-peer-address = 159.69.157.40
p2p-peer-address = 159.69.157.41
p2p-peer-address = 159.69.157.42
```
