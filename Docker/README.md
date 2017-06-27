### Run in docker

So simple and fast operation EOS:
 - [Docker](https://docs.docker.com)
 - [Docker-compose](https://github.com/docker/compose)
 - [Docker-volumes](https://github.com/cpuguy83/docker-volumes)

Build eos images

```
cd eos/Docker
cp ../genesis.json .
docker build -t eosio/eos .
```

Start docker

```
sudo mkdir -p /data/store/eos
docker-compose -f docker-compose.yml up
```

Done
