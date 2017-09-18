# Run in docker

Simple and fast setup of EOS on Docker is also available. 

## Install Dependencies
 - [Docker](https://docs.docker.com)
 - [Docker-compose](https://github.com/docker/compose)
 - [Docker-volumes](https://github.com/cpuguy83/docker-volumes)

## Build eos image

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos
cp genesis.json Docker 
docker build -t eosio/eos -f Docker/Dockerfile .
```

## Start docker

```bash
sudo rm -rf /data/store/eos # options 
sudo mkdir -p /data/store/eos
docker-compose -f Docker/docker-compose.yml up
```

If you get an error after `docker-compose` step, you may have to change directory permissions to your current system user 

```bash
sudo chown -R SYSTEM_USER /data/store/eos
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Execute eosc commands

You can run the `eosc` commands via `docker exec` command. For example:

```bash
docker exec docker_eos_1 eosc get info
```

```bash
docker exec docker_eos_1 eosc get account inita
```

Upload sample exchange contract 

```bash
docker exec docker_eos_1 eosc set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

## Access Shell of Running EOS Container

```bash
sudo docker exec -i -t docker_eos_1 /bin/bash
```