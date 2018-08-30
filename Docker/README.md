# Docker Setup

This guide explains how to get up and running with Docker.
Docker Hub image available from [docker hub](https://hub.docker.com/r/eosio/eos/).

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above)
- If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build EOS Containers:

Two options are provided:

1. Use the mainnet_setup.sh or testnet_setup.sh scripts
2. Manually build:

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker
docker build -t eosio/eos:latest .
```

- Both methods will use the .env file variables to build.

### .env

Within the eos/Docker directory, we store a .env containing the various variables to use in the build process. These can be modified to fit your needs:

```bash
$ cat eos/Docker/.env
EOS_TESTNET_VERSION=v1.1.0
EOS_MAINNET_VERSION=latest
EOS_SYMBOL=EOS
EOS_BRANCH=master
EOS_ORG_NAME=EOSIO # The org name is the EOSIO on the end of https://github.com/EOSIO, or your own forked url
```

## Start both nodeos and keosd containers (mainnet + latest):

```bash
docker volume create --name=nodeos-data-volume
docker volume create --name=keosd-data-volume
docker-compose -f docker-compose.yml up -d
```

- After running `docker-compose up -d`, two containers named `nodeos` and `keosd` will be started in the background (up -d for detach).
-- The nodeos container/servuce will expose ports 8888 and 9876.
-- The keosd container/service does not expose any port to the host. It does however allow you to alias and run cleos from the local machine: `alias cleos='docker exec keosd /opt/eosio/bin/cleos -u http://nodeos:8888 --wallet-url http://127.0.0.1:8900'

### (Alternative) Start nodeos docker container only:

```bash
docker run --name nodeosd -p 8888:8888 -p 9876:9876 -t eosio/eos:latest nodeosd.sh -e --http-alias=nodeos:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888
```

You can also directly mount a host directory into the container:

```bash
docker run --name nodeosd -v /path-to-data-dir:/opt/eosio/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos nodeosd.sh -e --http-alias=nodeos:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888
```

## Verify & Get chain info:

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

- Sometimes your nodeos container does not start after the compose or build process. You can check with `docker container ls -a` and manually start it.

## Execute cleos commands:

You can run the `cleos` commands via the bash alias we provided above:

```bash
cleos get info
cleos get account inita
```

Upload sample exchange contract

```bash
cleos set contract exchange contracts/exchange/
```

If you don't need keosd afterwards, you can stop the keosd service using

```bash
docker-compose stop keos
```

## Develop/Build custom contracts:

Due to the fact that the eosio/eos image does not contain the required dependencies for contract development (this is by design, to keep the image size small), you will need to utilize the eosio/eos-dev image. This image contains both the required binaries and dependencies to build contracts using eosiocpp.

You can either use the image available on [Docker Hub](https://hub.docker.com/r/eosio/eos-dev/) or navigate into the dev folder and build the image manually or with the setup script.

```bash
cd dev
./dev_setup.sh
```

## Change default configuration:

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  nodeos:
    volumes:
      - nodeos-data-volume:/opt/eosio/data-dir
      - ./config2.ini:/opt/eosio/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose -f docker-compose.yml down # You can change this if your original yml is different
docker-compose -f docker-compose.override.yml up -d
```

## Docker Environment Reset / Cleanup

By default, all data is persisted in a docker volume (under ~/). There is a script available for an entire reset of your docker environment: `./docker_reset.sh`.

## EOSIO Testnet:

We can easily set up a EOSIO local testnet with the testnet_setup.sh or with the commands below.

You'll need to place your genesis.json for the testnet into ~/nodeos-test-data-volume before the setup script will run.

Note: if you want to use the mongodb plugin, you have to enable it in your `config.ini`.

```
# Create volume
docker volume create --name=nodeos-test-data-volume
docker volume create --name=keosd-test-data-volume
# Pull images and start containers
docker-compose -f docker-compose-testnet.yml up -d
# Get/Verify chain info
curl http://127.0.0.1:8888/v1/chain/get_info
# Get logs
docker-compose logs -f nodeos_test
# stop containers
docker-compose -f docker-compose-testnet.yaml down
# Set alias 
alias dev_cleos='docker exec keos_test /opt/eosio/bin/cleos -u http://nodeos_test:8888 --wallet-url http://127.0.0.1:8900'
```

- The `blocks` data is stored under `--data-dir` by default. The wallet files are stored under `--wallet-dir` by default. These can be changed if necessary.

