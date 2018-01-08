# Run in docker

Simple and fast setup of EOS.IO on Docker is also available.

## Install Dependencies
 - [Docker](https://docs.docker.com) Docker 17.05 or higher is required

## Docker Requirement
 - At least 8GB RAM (Docker -> Preferences -> Advanced -> Memory -> 8GB or above)
 
## Build eos image

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker
docker build . -t eosio/eos
```

## Start eosiod docker container only

```bash
docker run --name eosiod -p 8888:8888 -p 9876:9876 -t eosio/eos start_eosiod.sh arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:
``` bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' eosiod
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container
```bash
docker run --name eosiod -v /path-to-data-dir:/opt/eosio/bin/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos start_eosiod.sh arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both eosiod and walletd containers

```bash
docker-compose up
```

After `docker-compose up`, two services named eosiod and walletd will be started. eosiod service would expose ports 8888 and 9876 to the host. walletd service does not expose any port to the host, it is only accessible to eosioc when runing eosioc is running inside the walletd container as described in "Execute eosioc commands" section.


### Execute eosioc commands

You can run the `eosioc` commands via a bash alias.

```bash
alias eosioc='docker-compose exec walletd /opt/eosio/bin/eosioc -H eosiod'
eosioc get info
eosioc get account inita
```

Upload sample exchange contract

```bash
eosioc set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

If you don't need walletd afterwards, you can stop the walletd service using

```bash
docker-compose stop walletd
```
### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  eosiod:
    volumes:
      - eosiod-data-volume:/opt/eosio/bin/data-dir
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
docker volume rm docker_eosiod-data-volume
```
