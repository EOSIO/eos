# CyberWay Testnet: How to setup and run BlockProducer node
## Hardware and software requirements
  - VDS: 1 vCPU, 2Gb Ram, 10Gb HDD should be enough
  - Ubuntu 16.04 or 18.04 recommended, any other system able to run docker images could work but untested.
  - [Docker](https://www.digitalocean.com/community/tutorials/how-to-install-and-use-docker-on-ubuntu-16-04) and [docker-compose](https://www.digitalocean.com/community/tutorials/how-to-install-docker-compose-on-ubuntu-16-04) are nessesary.

## Run node
Add your account into docker group to make things easier
```sh
sudo usermod -G docker -a $USER
```
Download docker configs and genesis data
```sh
mkdir ~/testnet
cd ~/testnet
wget https://raw.githubusercontent.com/GolosChain/cyberway/master/Docker/{docker-compose.yml,config.ini}
wget http://download.golos.io/genesis.tar.gz
tar -xf genesis.tar.gz
mv genesis genesis-data
```
Create file `genesis-data/genesis.json` with contents:
```json
{
  "initial_timestamp": "2019-03-01T12:00:00.000",
  "initial_key": "GLS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
  "initial_configuration": {
    "max_block_net_usage": 1048576,
    "target_block_net_usage_pct": 1000,
    "max_transaction_net_usage": 524288,
    "base_per_transaction_net_usage": 12,
    "net_usage_leeway": 500,
    "context_free_discount_net_usage_num": 20,
    "context_free_discount_net_usage_den": 100,
    "max_block_cpu_usage": 2500000,
    "target_block_cpu_usage_pct": 1000,
    "max_transaction_cpu_usage": 1800000,
    "min_transaction_cpu_usage": 100,
    "max_transaction_lifetime": 3600,
    "deferred_trx_expiration_window": 600,
    "max_transaction_delay": 3888000,
    "max_inline_action_size": 4096,
    "max_inline_action_depth": 4,
    "max_authority_depth": 6
  },
  "initial_chain_id": "0000000000000000000000000000000000000000000000000000000000000000"
}
```
Set some important values in config.ini:
```
p2p-peer-address = 116.203.104.164:9876 # seed node
p2p-listen-endpoint = 0.0.0.0:9876 # Allow incoming connections
producer-name = <Producer name> # change it to desired name of your producer
```
Create volumes and start it up:
```sh
docker volume create cyberway-mongodb-data
docker volume create cyberway-nodeos-data
docker-compose up -d
```
You can check if node is syncing by running
```sh
docker logs --tail 100 -f nodeosd
```
Output should be something like:
```text
Received block 048c051958858835... #166000 @ 2019-03-13T07:32:00.000 
```
Which means node is up and syncing. 

## Setup cleos docker image and create alias
We need cleos binary to interact with node.
Easy way to get it - install docker image.
```sh
docker run -ti -d --name keosd --net testnet_cyberway-net cyberway/cyberway:stable /opt/cyberway/bin/keosd
```
To make life easier put this line into your `~/.bashrc` and relogin
```
alias cleos='docker exec -ti keosd cleos --url http://nodeosd:8888 '
```
Now you can execute cleos by entering just `cleos`
For instance to get current blockchain status:
```sh
cleos get info
```

## Create validator keys, wallet and CyberWay account

To produce blocks we need a few more things.
First, blockproducer keypair (its different from wallet keypair, just BP stuff). Lets generate it by running
```sh
cleos create key --to-console
```
and put into corresponding config.ini param
```text
signature-provider = <Public key>=KEY:<Private key>
```

Node restart is needed
```sh
docker-compose down
docker-compose up -d
```

Let's create wallet next
```sh
cleos wallet create --file wallet.pass
```
Use this command to unlock it in the future: 
```sh
cleos wallet unlock --password `docker exec -ti keosd cat wallet.pass`
```
Now we need to create a new key for this wallet. 
```sh
cleos wallet create_key
```
Command above will return public key to stdout.
You will need it to create account in the CyberWay network.

CyberWay is EOS fork, so you need to ask peoples on [t.me/CyberWayOS](https://t.me/CyberWayOS) Telegram Channel to register one for you.

Just drop desired name (it should be same as `<Producer Name>` you set above in config.ini) and wallet public key into chatroom and someone will help you.

## Register blockproducer
Ensure you node is in sync first by checking logs.
```sh 
docker logs --tail 100 -f nodeosd
```
If its ok, run:
```sh
cleos push action cyber.stake setproxylvl '{"account":"<producer-name>", "token_code":"SYS", "purpose_code":"", "level":0}' -p <producer-name>
cleos push action cyber.stake setkey '{"account":"<producer-name>", "token_code":"SYS", "signing_key":"<public-key>"}' -p <producer-name>
```
Replace `<public-key>` and `<producer-name>` with values from config.ini.

Wait for a 15 minutes and check if your account name in there:
```sh
cleos get account cyber.prods
```
***Congratulations, you are CyberWay BlockProducer now!***

# Useful links
   - [Cleos commands available](https://gist.github.com/vikxx/dfa9d671beb07a39e6eb18bad2c1174e)
   - [Основные особенности блокчейна](https://golos.io/ru--blokcheijn/@goloscore/cyberway-predposylki-sozdaniya-platformy-osnovnye-otlichiya-ot-eos)
  - [Дорожная карта проекта](https://golos.io/ru--blokcheijn/@goloscore/cyberway-predposylki-sozdaniya-platformy-osnovnye-otlichiya-ot-eos)
  - [Базовые положения](https://golos.io/ru--blokcheijn/@goloscore/cyberway-predposylki-sozdaniya-platformy-osnovnye-otlichiya-ot-eos)
  - [Информация о тестнете](https://golos.io/ru--blokcheijn/@goloscore/novosti-golos-core-testnet-eksperimentalnaya-versiya)
  - [Тестнет. Руководство по установке для блок-продюсеров](https://golos.io/cyberway/@goloscore/cyberway-testnet-rukovodstvo-po-ustanovke-dlya-blok-prodyuserov)
