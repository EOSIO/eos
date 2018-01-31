# Run in docker

Simple and fast setup of EOS.IO on Docker is also available.

- [Install Dependencies](#install-dependencies)
- [Docker Requirement](#docker-requirement)
- [Build eosio image](#build-eosio-image)
- [Start both eosiod and walletd containers with docker-compose](#start-both-eosiod-and-walletd-containers-with-docker-compose)
  - [Execute eosioc commands](#execute-eosioc-commands)
  - [Making RPC API Call](#making-rpc-api-call)
  - [Change eosiod default configuration](#change-eosiod-default-configuration)
  - [Clear data-dir](#clear-data-dir)
- [Start eosiod docker container only](#start-eosiod-docker-container-only)

## Install Dependencies
 - [Docker](https://docs.docker.com) Docker 17.05 or higher is required

## Docker Requirement
 - At least 8GB RAM (Docker -> Preferences -> Advanced -> Memory -> 8GB or above)
 
## Build eosio image

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker
docker build . -t eosio/eos
```

## Start both eosiod and walletd containers with docker-compose

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

Upload sample currency contract

```bash
eosioc wallet create
eosioc wallet import 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
eosioc create account inita currency EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
eosioc set contract currency contracts/currency/currency.wast contracts/currency/currency.abi
```

If you don't need walletd afterwards, you can stop the walletd service using

```bash
docker-compose stop walletd
```

### Making RPC API Call

Port 8888 of eosiod container is linked to port 8888 of host
```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

### Change eosiod default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  eosiod:
    volumes:
      - ./config2.ini:/opt/eosio/bin/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```
This will replace the `config.ini` inside your docker container with `config2.ini`

### Clear data-dir
The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm docker_eosiod-data-volume
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
docker run --name eosiod -v /absolute-path-to-data-dir:/opt/eosio/bin/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos start_eosiod.sh arg1 arg2
```
The mounted host directory contains `config.ini` that is used by eosiod. After it is mounted, you can modify `config.ini` locally and restart your docker container to see the effect.

