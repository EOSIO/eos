# Run in docker

Simple and fast setup of EOS.IO on Docker is also available.

## Install Dependencies
 - [Docker](https://docs.docker.com) Docker 17.05 or higher is required
 - [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement
 - At least 8GB RAM (Docker -> Preferences -> Advanced -> Memory -> 8GB or above)

## Build eos image

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker
docker build . -t eosio/eos
```

## Start nodeos docker container only

```bash
docker run --name nodeos -p 8888:8888 -p 9876:9876 -t eosio/eos nodeosd.sh arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:
``` bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' nodeos
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container
```bash
docker run --name nodeos -v /path-to-data-dir:/opt/eosio/bin/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos nodeosd.sh arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both nodeos and keosd containers

```bash
docker volume create --name=nodeos-data-volume
docker volume create --name=keosd-data-volume
docker-compose up -d
```

After `docker-compose up -d`, two services named `nodeosd` and `keosd` will be started. nodeos service would expose ports 8888 and 9876 to the host. keosd service does not expose any port to the host, it is only accessible to cleos when runing cleos is running inside the keosd container as described in "Execute cleos commands" section.


### Execute cleos commands

You can run the `cleos` commands via a bash alias.

```bash
alias cleos='docker-compose exec keosd /opt/eosio/bin/cleos -H nodeosd'
cleos get info
cleos get account inita
```

Upload sample exchange contract

```bash
cleos set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

If you don't need keosd afterwards, you can stop the keosd service using

```bash
docker-compose stop keosd
```
### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  nodeos:
    volumes:
      - nodeos-data-volume:/opt/eosio/bin/data-dir
      - ./config2.ini:/opt/eosio/bin/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```

### Clear data-dir
The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm nodeos-data-volume
docker volume rm keosd-data-volume
```
