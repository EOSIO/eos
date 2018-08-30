# Docker Setup

This guide explains how to get up and running with Docker.
Docker Hub image available from [docker hub](https://hub.docker.com/r/eosio/eos/).

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Requirements

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above). If the build below fails, make sure you've adjusted Docker Memory settings and try again.
- Ability to edit a text file and execute a bash script

## Building the EOS Containers:

There are currently three Docker template directories you can use: `latest`, `testnet`, and `dev`. Inside of `latest`, you'll see a setup.sh, docker-compose.yml, Dockerfile, and a hidden .env file. 

If you look at the .env file, you'll see (which are also the defaults used in the Dockerfile for manual building):

```bash
$ cat eos/Docker/latest/.env
# GITHUB
GITHUB_BRANCH=master
GITHUB_ORG=EOSIO # You can find this on the github url: https://github.com/EOSIO
# COMPOSE / BUILD
DOCKERHUB_USERNAME=eosio
IMAGE_NAME=eos
IMAGE_TAG=latest
BUILD_SYMBOL=SYS
NODEOS_VOLUME_NAME=nodeos-data-volume
NODEOS_PORT=9876
NODEOS_API_PORT=8888
KEOSD_VOLUME_NAME=keosd-data-volume
KEOSD_API_PORT=8900
COMPOSE_PROJECT_NAME=main
```

These are variables used throughout the scripts, allowing you to simply copy the directory and modify the .env file if you wish to setup a second instance (like running on a public testnet). If you modify these values, like the BUILD_SYMBOL=SYS to =EOS, running `./setup.sh` will set your containers up with that as the "CORE symbol name". You can see that the dev directory has a slightly different .env file if you need more examples.

If you don't wish to use the setup.sh script, you can manually run docker build. Defaults are included in the Dockerfile, but you can override them with `--build-arg`. 

*The .env is not used when manually building!*

Here is an example:

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker/latest
docker build -t eosio/eos:v1.1.0 --build-arg COMPOSE_PROJECT_NAME=testnet --build-arg BUILD_SYMBOL=LEOS --build-arg GITHUB_BRANCH=master --build-arg GITHUB_ORG=EOSIO --no-cache .
```

To set up and start the containers, you can run:

```bash
cd eos/Docker/latest
./setup.sh
```

- After running the setup script, two containers named `main_nodeos` and `main_keosd` (if you have `COMPOSE_PROJECT_NAME=main`) will be started in the background (`up -d` = create & detach).
-- The nodeos container/service will expose ports 8888 and 9876.
-- The keosd container/service does not expose any port to the host. It does however allow you to alias and run cleos from the local machine: `alias cleos='docker exec main_keosd /opt/eosio/bin/cleos -u http://main_nodeos:8888 --wallet-url http://127.0.0.1:8900'


### Verify your installation

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

Alternatively, you can run:

```bash
cleos get info
cleos get account eosio
```

## (Alternative method) Start nodeos docker container only:

```bash
docker run --name nodeosd -p 8888:8888 -p 9876:9876 -t eosio/eos:latest nodeosd.sh -e --http-alias=nodeos:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888
```

You can also directly mount a host directory into the container:

```bash
docker run --name nodeosd -v /path-to-data-dir:/opt/eosio/data-dir -p 8888:8888 -p 9876:9876 -t eosio/eos nodeosd.sh -e --http-alias=nodeos:8888 --http-alias=127.0.0.1:8888 --http-alias=localhost:8888
```

- Sometimes the nodeos container does not start after the compose or build process. You can check with `docker container ls -a` and manually start it.

## Develop/Build custom contracts:

Due to the fact that the eosio/eos image does not contain the required dependencies for contract development (this is by design, to keep the image size small), you will need to utilize the eosio/eos-dev image. This image contains both the required binaries and dependencies to build contracts using eosiocpp.

You can either use the image available on [Docker Hub](https://hub.docker.com/r/eosio/eos-dev/) or navigate into the dev folder and build the image manually or with the setup script.

```bash
cd dev
./dev_setup.sh
```

## Change default configuration

You can modify the docker-compose.yml file to change the default configurations. For example, add a new local file: `config2.ini`, and point it to the docker container's data-dir:

```yaml
version: "3"

services:
  nodeos:
. . .
    expose:
      - "8888"
    volumes:
      - ~/$NODEOS_VOLUME_NAME:/opt/eosio/bin/data-dir
      - ~/some_dir/config2.ini:/opt/eosio/data-dir/config.ini
```

Now, when you modify the ~/some_dir/config2.ini locally, the changes will be included in the docker container, overwriting the config that was used before. This requires a reboot of the containers:

```bash
cd eos/Docker/latest
docker-compose down
docker-compose up -d
```

Remember that docker-compose requires a docker-compose.yml in the same directory. You can use `docker-compose -f docker-compose-new.yml` if you want to specify a different file.

## Docker Environment Reset / Cleanup

By default, all data is persisted in a docker volume under ~/. There is a script available for an entire reset of your docker environment: `./docker_reset.sh`. It will prompt you before executing just to be safe.

## EOSIO Testnet:

We can easily set up a EOSIO local testnet by making a copy of the `latest` directory. Once copied, modify the .env values to match the version and use a different COMPOSE_PROJECT_NAME so it doesn't conflict with other containers. You can setup a second alias for the new container like so:

`alias test_cleos='docker exec test_keosd /opt/eosio/bin/cleos -u http://test_nodeos:8888 --wallet-url http://127.0.0.1:8900'`

## Miscellaneous / Notes
- The blocks/state data is stored under --data-dir by default. The wallet files are stored under --wallet-dir by default. These can be changed in you docker-compose.yml
- Take regular backups of your volume directories
- If using a public blockchain, you need to wait for the entire blockchain to catch up/sync to the latest blocks before you can perform actions through your keosd/nodeos. Alternatively, you can run keosd on its own and set the cleos `-u` to a public api of an up to date/synced producer
