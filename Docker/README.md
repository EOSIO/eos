### Run in docker

Simple and fast setup of EOS on Docker is also available. Firstly, install dependencies:
 - [Docker](https://docs.docker.com)
 - [Docker-compose](https://github.com/docker/compose)
 - [Docker-volumes](https://github.com/cpuguy83/docker-volumes)

Build eos image

```
git clone https://github.com/EOSIO/eos.git --recursive
cd eos
cp genesis.json Docker 
docker build -t eosio/eos -f Docker/Dockerfile .
```

Start docker

```
sudo rm -rf /data/store/eos # options 
sudo mkdir -p /data/store/eos
docker-compose -f Docker/docker-compose.yml up
```

Get chain info

```
curl http://127.0.0.1:8888/v1/chain/get_info
```

Run example contracts

You can run the `eosc` commands via `docker exec` command. For example:
```
cd /data/store/eos/contracts/exchange
docker exec docker_eos_1 eosc contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi

```

Done
