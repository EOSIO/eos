#statetrack_plugin

This plugin adds hooks on chainbase operations emplace, modify, remove, undo, squash and commit and routes the objects and revision numbers to a ZMQ queue.

Configuration
The following configuration statements in config.ini are recognized:

plugin = eosio::statetrack_plugin -- enables the state track plugin

st-zmq-sender-bind = ENDPOINT -- specifies the PUSH socket connect endpoint. Default value: tcp://127.0.0.1:3000.

#adding library on Mac

#adding library on Ubuntu

```shell
apt-get install -y pkg-config libzmq5-dev
```

#Building 

