### Run in docker

So simple and fast operation EOS:
 - [Docker](https://docs.docker.com)
 - [Docker-compose](https://github.com/docker/compose)
 - [Docker-volumes](https://github.com/cpuguy83/docker-volumes)

Build eos images

```
cd eos/Docker
cp ../genesis.json .
docker build --rm -t eosio/eos .
```

Start docker

```
sudo rm -rf /data/store/eos # options 
sudo mkdir -p /data/store/eos
docker-compose -f docker-compose.yml up
```

Run example contracts

```
cd /data/store/eos/contracts/exchange
docker exec docker_eos_1 eosc setcode exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi

cd /data/store/eos/contracts/currency 
docker exec docker_eos_1 eosc setcode currency contracts/currency/currency.wast contracts/currency/currency.abi

```

Done
