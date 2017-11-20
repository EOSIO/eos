# Run in docker

Simple and fast setup of EOS on Docker is also available.

## Install Dependencies
 - [Docker](https://docs.docker.com) Docker 17.05 or higher is required

## Build eos image

```bash
git clone https://github.com/EOSIO/eos.git --recursive
cd eos/Docker
docker build . -t eosio/eos
```

## Start eosd docker container only

```bash
docker run -p 8888:8888 -p 9876:9876 -t eosio/eos start_eosd.sh arg1 arg2
```

## Start both eosd and walletd containers

```bash
docker-compose up
```

After `docker-compose up`, two services named eosd and walletd will be started. eosd service would expose ports 8888 and 9876 to the host. walletd service does not expose any port to the host, it is only accessible to eosc when runing eosc is running inside the walletd container as described in "Execute eosc commands" section.

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Execute eosc commands

You can run the `eosc` commands via a bash alias.

```bash
alias eosc='docker-compose exec walletd /opt/eos/bin/eosc -H eosd'
eosc get info
eosc get account inita
```

Upload sample exchange contract

```bash
eosc set contract exchange contracts/exchange/exchange.wast contracts/exchange/exchange.abi
```

If you don't need walletd afterwards, you can stop the walletd service using

```bash
docker-compose stop walletd
```
## Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  eosd:
    volumes:
      - eosd-data-volume:/opt/eos/bin/data-dir
      - ./config2.ini:/opt/eos/bin/data-dir/config.ini
```

Then restart your docker containers like.

```bash
docker-compose down
docker-compose up
```
