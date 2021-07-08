---
content_title: How-To Setup And Use Fill-Queue
link_text: How To Setup and Use Fill-Queue
---

## Summary

This how-to demonstrates the set up and use of [Fill-Queue application](index) running locally.

## Prerequisites

Before you begin, make sure you have the following information or have completed the following referenced procedures:

1. `nodeos` runs locally.
2. `nodeos` has the state history plugin enabled. \
For instructions on how to start `nodeos` with history plugin enabled consult
[the following documentation](https://developers.eos.io/manuals/eos/v2.1/nodeos/plugins/state_history_plugin/index/).
3. RabbitMQ runs locally.
4. OpenSSL is installed locally.
5. Simple-amqp-client is installed locally.

### RabbitMQ Installation

To install and start RabbitMQ along with the rest of dependencies run the following commands.

#### For OSX

```sh
brew install openssl rabbitmq simple-amqp-client
export PATH=$PATH:/usr/local/opt/rabbitmq/sbin
rabbitmq-server
```

#### For Linux

```sh
apt update && apt upgrade -y
apt install openssl libssl-dev librabbitmq-dev wget build-essential cmake libboost-all-dev git autoconf2.13 clang-7 libgmp-dev liblmdb-dev wget curl libcurl4-openssl-dev python3-pkgconfig rabbitmq-server -y Rabbitmq-server
wget https://github.com/EOSIO/cppKin/releases/download/v1.1.1/cppkin_1.1.1-ubuntu-20.04_amd64.deb
apt install -y ./cppkin_1.1.1-ubuntu-20.04_amd64.deb
wget https://github.com/alanxz/SimpleAmqpClient/archive/v2.4.0.tar.gz
tar xfv v2.4.0.tar.gz
cd SimpleAmqpClient-2.4.0
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
cmake --build . --target install
```

## Procedure

Complete the following steps to setup and use Fill-Queue.

### 1. build Fill-Queue from source code

#### For OSX

```sh
export LIBRARY_PATH="/usr/local/opt/icu4c/lib:${LIBRARY_PATH}"
git clone git@github.com:EOSIO/fill-queue.git
cd fill-queue
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j
```

#### For Linux

```sh
git clone git@github.com:EOSIO/fill-queue.git
cd fill-queue
mkdir build && cd build
cmake ..
make -j
```

### 2. Run Fill-Queue

```sh
cd build
./fill-queue --data-dir ../data --config-dir ../
```

## Next Steps

The following options are available when you have completed the procedure:

* See the first messages of the queue with:

```sh
rabbitmqadmin get queue=fq.default
```

* Stop Fill-Queue with:

```sh
ctrl+c to end process (the queue will have all the relevant messages)
```

* Make use of addition configuration parameters: see the section [Configuration Parameters Settings](./index#configuration-parameters-settings) in the explainer page.
