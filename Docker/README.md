# Run in docker

Simple and fast setup of EOS on Docker is also available.

## Install Dependencies
 - [Docker](https://docs.docker.com) Docker 17.05 or higher is required

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
sudo docker run -v /data/store/eos:/opt/eos/bin/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos
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