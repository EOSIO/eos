# block-producer-swap-bot

  * [Getting started](#getting-started)
    * [Requirements](#getting-started-requirements)
      * [Ubuntu 16.04 & 18.04](#ubuntu-1604--1804)
  * [Block producers](#block-producers)
    * [Configuration file](#configuration-file)
    * [Usage](#usage)
  
## Getting started

<h3 id="getting-started-requirements">Requirements</h4>

#### Ubuntu 16.04 & 18.04

Go to swap-bot-directory:

```bash
$ cd block-producer-swap-bot
```

Install requirements with following commands:

```bash
$ pip3 install requirements.txt
```

## Block producers

### Configuration file

Open configuration file.

```bash
$ nano cli/config.ini
```

Paste into the config file following content:

```
[NODES]
nodeos-url=http://127.0.0.1:8888
eth-provider=wss://rinkeby.infura.io/ws/v3/3f98ae6029094659ac8f57f66e673129
[REMIO]
block-producer-permission=producer111a@active
```

Replace nodeos-url, eth-provider, block-producer-permission with your nodeos url, link to Ethereum node with websocket connection and your account for signing .


### Usage

To start approving swaps run the following command

```bash
$ python3 block_producer_swap_bot.py eth-swap-bot process-swaps
```
