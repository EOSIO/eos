# block-producer-swap-bot

  * [Getting started](#getting-started)
    * [Requirements](#getting-started-requirements)
      * [Ubuntu 16.04 & 18.04](#ubuntu-1604--1804)
  * [Block producers](#block-producers)
    * [Configuration file](#configuration-file)
    * [Usage](#usage)
  * [Development](#development)
  
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

## Development

Manually process particular swap transaction
```bash
$ cleos --url http://127.0.0.1:8000 --wallet-url http://127.0.0.1:6666 push action remio.swap processswap '[ "producer111a", "0x30a9479fc792d3219aba23440235a4a7e4ab32e7ff86a08d878778c5076d206b", "1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32", "EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC", "20.0000 REM", "2019-07-25T17:51:47" ]' -p producer111a@active
```

Manually finish swap
```bash
cleos --url http://127.0.0.1:8000 --wallet-url http://127.0.0.1:6666 push action remio.swap finishswap '[ "receiver", "0x30a9479fc792d3219aba23440235a4a7e4ab32e7ff86a08d878778c5076d206b", "1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32", "EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC","20.0000 REM", "2019-07-25T17:51:47", "SIG_K1_AnEQt8cs8Uscfg3FhRM3za3WxzzP8VHcUCAhfwoQYUb8EFbEujrTswAeiCaNWKzSfADWzVBoBv3mxV7qb4ymo4QHXFfTCLQWd6kH45WL2KggUP4aTpvH", "None", "EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC", "EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC" ]' -p producer111a@active
```bash
