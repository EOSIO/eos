# statetrack_plugin

This plugin adds hooks on chainbase operations emplace, modify, remove, undo, squash and commit and routes the objects and revision numbers to a ZMQ queue.

### Configuration

The following configuration statements in config.ini are recognized:

plugin = eosio::statetrack_plugin -- enables the state track plugin

st-zmq-sender-bind = ENDPOINT -- specifies the PUSH socket connect endpoint. Default value: tcp://127.0.0.1:3000.

### adding ZMQ library on Mac

```
brew tap jmuncaster/homebrew-header-only
brew install jmuncaster/header-only/cppzmq
```

### adding ZMQ library on Ubuntu

```
apt-get install -y pkg-config libzmq5-dev
```

### Building 

```
./eosio_build.sh
./eosio_install.sh
```
