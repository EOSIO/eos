## EOS.IO 中文引导

略过官方的一些介绍，直接开始。在 [wiki](https://github.com/EOSIO/eos/wiki/Testnet%3A%20Public) 中所说的公开网络测试使用 `dawn-2.x` 分支.  `master` 分支暂时只用于本地网络测试.  下边首先介绍的内容基于 `master` 分支中的本地网络测试。文档会在 `dawn-3.x` 发布后，更新公开网络测试环境引导手册。

注：当前本文重点为，本地环境搭建与基本使用。各个名词、CLI、详细的解释请参考 WIKI，或等待译文。

### 支持的操作系统
EOS.IO 目前支持以下操作系统：
1. Amazon 2017.09 或者更高。
2. Centos 7。
3. Fedora 25 或者更高 (建议使用 Fedora 27)。
4. Mint 18。
5. Ubuntu 16.04 (建议使用 Ubuntu 16.10)。
6. MacOS Darwin 10.12 或者更高 (建议使用 MacOS 10.13.x)。

# 资源
1. [EOS.IO 官方网站](https://eos.io)
2. [文档](https://eosio.github.io/eos/)
3. [博客](https://steemit.com/@eosio)
4. [Telegram 交流群](https://t.me/EOSProject)
5. [Telegram 开发交流群](https://t.me/joinchat/EaEnSUPktgfoI-XPfMYtcQ)
6. [白皮书](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md)
8. [Wiki](https://github.com/EOSIO/eos/wiki)

# 目录

1. [开始使用](#gettingstarted)
2. [配置构建、开发环境](#setup)
   1. [自动化构建脚本](#autobuild)
      1. [在 Linux (Amazon, Centos, Fedora, Mint, & Ubuntu) 上使用本地测试环境](#autoubuntulocal)
      2. [在 Linux (Amazon, Centos, Fedora, Mint, & Ubuntu) 上使用公开测试环境](#autoubuntupublic)
      3. [MacOS 本地测试环境](#automaclocal)
      4. [MacOS 公开测试环境](#automacpublic)
3. [构建 EOS 并运行一个节点](#runanode)
   1. [下载代码](#getcode)
   2. [源码编译](#build)
   3. [运行单节点本地测试](#singlenode)
   4. [接下来](#nextsteps)
4. [简单的货币合约例子](#smartcontracts)
   1. [运行节点](#smartcontractexample)
   2. [配置钱包并导入账号私钥](#walletimport)
   3. [创建合约使用的账户](#createaccounts)
   4. [上传示例合约到区块链](#uploadsmartcontract)
   5. [一些操作](#pushamessage)
   6. [读取信息](#readingcontract)
5. [多节点本地测试](#localtestnet)
6. [在公开环境进行测试](#publictestnet)

<a name="gettingstarted"></a>
## 开始
目前文档中仅详细描述了本地测试环境的操作指引（公开测试环境大同小异），由于英文文档步骤暂时与实际操作步骤不同，所以详细的出一篇中文的译本，并纠正一些细节，更好的让广大开发者进行 `EOS` 开发的学习。

<a name="setup"></a>
## 配置编译、开发环境

<a name="autobuild"></a>
### 自动构建脚本

递归克隆项目，并运行 `eosio_build.sh ` 即可。

:warning: **As of February 2018, `master` is under heavy development and is not suitable for experimentation.** :warning:

我们推荐跟着操作指引构建公开网络测试环境（个人建议在本地测试环境下先进行理解）。

<a name="autoubuntulocal"></a>

#### 本地测试环境

```bash
$ git clone https://github.com/eosio/eos --recursive
$ cd eos
$ ./eosio_build.sh

$ cd build
$ make test
$ sudo make install
```

进入下一步：[创建并且运行单节点测试](#singlenode)。

<a name="autoubuntupublic"></a>
#### 公开测试环境

目前依旧在 `dawn-2.x` 分支下，`3.x` 不久之后就会更新.  希望基于当前版本进行公开环境测试，跳转到：[DAWN-2018-02-14/eos/README.md](https://github.com/EOSIO/eos/blob/DAWN-2018-02-14/README.md)。

<a name="automaclocal"></a>
#### MacOS 本地测试环境

使用前，确定您的 MacOS 安装了 `Xcode` 以及 `Homebrew` ，作为开发者的你应该不需要教学。

```bash
$ git clone https://github.com/eosio/eos --recursive
$ cd eos
$ ./eosio_build.sh

$ cd build
$ make test
$ sudo make install
```

进入下一步：[创建并且运行单节点测试](#singlenode)。

<a name="automacpublic"></a>
#### MacOS 公开测试环境

目前依旧在 `dawn-2.x` 分支下，`3.x` 不久之后就会更新.  希望基于当前版本进行公开环境测试，跳转到：[DAWN-2018-02-14/eos/README.md](https://github.com/EOSIO/eos/blob/DAWN-2018-02-14/README.md)。

<a name="runanode"></a>
## 构建 EOS 并且运行节点

<a name="getcode"></a>
### 递归克隆代码

```bash
$ git clone https://github.com/eosio/eos --recursive
```

如果克隆的时候忘记使用 `--recursive` 参数，在主目录下使用下边的方式进行依赖下载。

```bash
$ git submodule update --init --recursive
```

<a name="build"></a>
### 源码构建

源码构建不做赘述。

<a name="singlenode"></a>

### 创建并启动单节点测试

构建成功之后，接下来你需要的内容主要在 `build/programs/nodeos` 文件夹下。`nodeos` 可以通过 `programs/nodeos/nodeos` 来运行。

 默认情况下，`etc/eosio/node_00` 是节点默认的配置文件夹，这个文件夹下有一个 `genesis.json` 文件。当然，配置文件也可以手动通过 `--config-dir` 指定所在的目录。

节点启动的时候，如果没有 `config.ini` 文件，将会创建一个默认的配置文件。如果你此时还没有配置文件，直接运行 `./nodeos` 将会创建一个，然后更新其中的内容为以下：

```
# 测试环境下的配置文件，包含一些初始化的 KEY 等值，注意更改其位置，测试节点使用默认位置(主目录下的 genesis.json 文件)
genesis-json = /path/to/eos/source/genesis.json
 # 由于单节点测试，开启 `陈旧` 模式
enable-stale-production = true
# 配置一个块生产者
producer-name = eosio

# 配置一个默认的 KEY PAIR

private-key = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]

# 加载区块链插件

# 生产者
plugin = eosio::producer_plugin
# 钱包
plugin = eosio::wallet_api_plugin
# CHAIN API 以及 HTTP 接口提供
plugin = eosio::chain_api_plugin
plugin = eosio::http_plugin
```

在上述配置文件中需要解释的点：

1. `private-key`，这个参数需要特别注意，在测试环境下，我们之后需要创建账户，这对 KEY 是项目默认提供的，后面要用到。信息可以在主目录下 `testnet.md` 中找到：

```json
"keys": [{
            "public_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "wif_private_key": "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
          }
        ],
```

现在，执行命令，因为我们刚刚生成了 `config.ini`，所以当前的目录结构大致如下：

```
nodeos
├── CMakeFiles
│   ├── ...
├── CTestTestfile.cmake
├── Makefile
├── cmake_install.cmake
├── config.hpp
├── config.ini
└── nodeos
```

故，执行 `./nodeos --config-dir .`，指定配置文件目录为当前目录下：

```
...
2442417ms thread-0   http_plugin.cpp:242           add_handler          ] add api url: /v1/txn_test_gen/start_generation
2442417ms thread-0   http_plugin.cpp:242           add_handler          ] add api url: /v1/txn_test_gen/stop_generation
2442505ms thread-0   chain_controller.cpp:1092     validate_block_heade ] head_block_time 2018-03-29T16:41:31.500, next_block 2018-03-29T17:40:42.500, block_interval 500
2442505ms thread-0   chain_controller.cpp:1094     validate_block_heade ] Did not produce block within block_interval 500ms, took 3551000ms)
eosio generated block 028c5923... #7238 @ 2018-03-29T17:40:42.500 with 0 trxs
eosio generated block 6883c717... #7239 @ 2018-03-29T17:40:43.000 with 0 trxs
eosio generated block 1a31062a... #7240 @ 2018-03-29T17:40:43.500 with 0 trxs
eosio generated block 72a4152f... #7241 @ 2018-03-29T17:40:44.000 with 0 trxs
```

默认情况下， `nodeos` 使用 `var/lib/eosio/node_00` 作为数据文件夹 (存储一些信息)。  同样，也可以使用 `--data-dir` 指定数据目录。

<a name="nextsteps"></a>
### 下一步

开始部署智能合约并测试吧！

<a name="smartcontracts"></a>
## 货币合约展示

官方默认提供了货币类型的合约：`currency`。

<a name="smartcontractexample"></a>
### 首先，运行节点

还记得刚刚如何运行节点的么？

<a name="walletimport"></a>
### 配置一个钱包，并且导入账户私钥

请确认你在 `nodeos` 的配置中加入了 `plugin = eosio::wallet_api_plugin`，EOS 钱包将会是 `nodeos` 进程的一部分。每个合约都需要关联一个账户，所以我们先创建一个钱包：

```bash
$ cd ~/eos/build/programs/cleos/
$ ./cleos wallet create -n ${your wallet name}
```

请保存生成的密钥，用于解锁/锁定你的账户。接下来，为确保账户已经解锁：

```bash
$ ./cleos wallet unlock -n ${your wallet name}	
password: # 在这里直接粘贴生成的密钥，直接按回车即可
password: Unlocked: ${your wallet name}	
```

导入我们之前在 `config.ini` 中预先设置的一对 KEY 的私钥。注意，**不是**以 EOS 开头。

```bash
$ ./cleos wallet import 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
```

<a name="createaccounts"></a>
### 为合约创建一个账户

每个账户都需要两对 KEY， `owner_key` 和 `active_key`.

```bash
$ cd ~/eos/build/programs/cleos/
```

```bash
$ ./cleos create key
Private key: 5JvjBgi9vHKjmhGqYhqxvq9pZw9fkrEEUBRnRT3VAuAjPE3ZamS
Public key: EOS57GaBRSYLaZpfPJjSaorx4YM14EscVW9q1bTzjzyaJhE9vknMK
$ ./cleos create key
Private key: 5JeMKSY1GbETSia7SP3UMBz2h1wPPREZUzxkEW6i1rrVdYDFMZ8
Public key: EOS7iLfKWDgBfyeh3Z5TXEasyQkpkTfdzNXUyYBLJZXhuNzEEvF3u
```

接下来，我们需要将这两对 KEY 依次导入到我们的钱包中：

```bash
$ ./cleos wallet import 5JeMKSY1GbETSia7SP3UMBz2h1wPPREZUzxkEW6i1rrVdYDFMZ8 -n ${your wallet name}
$ ./cleos wallet import 5JvjBgi9vHKjmhGqYhqxvq9pZw9fkrEEUBRnRT3VAuAjPE3ZamS -n ${your wallet name}
```

接下来，创建用户：

```bash
$ ./cleos create account eosio ${your account name} ${生成的 KEY1} ${生成的 KEY2} 
```

你将会看到这样的大串 JSON 返回：

```json
{
  "transaction_id": "1d77719a161b087acfd278631958e96d71e787b0d215570b589eb6c4e2db5606",
  "processed": {
    "status": "executed",
    "id": "1d77719a161b087acfd278631958e96d71e787b0d215570b589eb6c4e2db5606",
    "action_traces": [{
        "receiver": "eosio",
        "act": {
          "account": "eosio",
          "name": "newaccount",
          "authorization": [{
              "actor": "eosio",
              "permission": "active"
            }
          ],
          "data": "0000000000ea3055000000000090284d01000000010003740ab1de8c1754bc191c67b14980e773d2ac05f4d0dab71d433f6edfba2b7503010000010000000100021d048aa7762657a298a14b61185836c34f361cc01589714742353fe724bde6700100000100000000010000000000ea305500000000a8ed32320100"
        },
        "console": "",
        "region_id": 0,
        "cycle_index": 1,
        "data_access": [{
            "type": "write",
            "code": "eosio",
            "scope": "eosio.auth",
            "sequence": 2
          }
        ]
      }
    ],
    "deferred_transaction_requests": []
  }
}
```

做下检查：

```bash
$ ./cleos get account ${your account name}
```

如果你看到如下的 JSON，那么说明你成功了：

```json
{
  "account_name": ${your account name},
  "permissions": [{
      "perm_name": "active",
      "parent": "owner",
      "required_auth": {
        "threshold": 1,
        "keys": [{
            "key": "EOS57GaBRSYLaZpfPJjSaorx4YM14EscVW9q1bTzjzyaJhE9vknMK",
            "weight": 1
          }
        ],
        "accounts": []
      }
    },{
      "perm_name": "owner",
      "parent": "",
      "required_auth": {
        "threshold": 1,
        "keys": [{
            "key": "EOS7iLfKWDgBfyeh3Z5TXEasyQkpkTfdzNXUyYBLJZXhuNzEEvF3u",
            "weight": 1
          }
        ],
        "accounts": []
      }
    }
  ]
}
```

<a name="uploadsmartcontract"></a>
### 上传智能合约到本地

在上传之前，先确认当前账号没有合同：

```bash
$ ./cleos get code ${your account name}
code hash: 0000000000000000000000000000000000000000000000000000000000000000
```

关联我们创建的用户，并上传合约到本地网络中：

```bash
$ ./cleos set contract ${your account name} ../../contracts/currency/currency.wast ../../contracts/currency/currency.abi
```

收到大段的JSON返回说明创建成功，接下来我们来验证一下账户是否关联合约：

```bash
$ ./cleos get code ${your account name} 
```

将会返回如下：
```bash
code hash: 318deb7c9890ac8c75314072fa1bb9cd2e8aa2d6c150160c519a85c80f66eb58
```

在使用上传的智能合约之前，需要先创建（这些 action 可以在 currency.abi 中找到，详细的请了解 智能合约相关的 wiki）:

```bash
$ ./cleos push action ${your account name}  create '{"issuer": ${your account name}, "maximum_supply": "1000000000.0000 CUR", "can_freeze": 1, "can_recall": 1, "can_whitelist": 1}' --permission  ${your account name} @active
```

执行后收到类似如下JSON，即为创建成功（只能创建一次）：

```json
{
  "transaction_id": "132dc63a0bb9e33c31b1ddb3576aa3178ecf946c77efb99a0e6eb7ea7417f9d1",
  "processed": {
    "status": "executed",
    "id": "132dc63a0bb9e33c31b1ddb3576aa3178ecf946c77efb99a0e6eb7ea7417f9d1",
    "action_traces": [{
        "receiver": "${your account name}",
        "act": {
          "account": "${your account name}",
          "name": "create",
          "authorization": [{
              "actor": "${your account name}",
              "permission": "active"
            }
          ],
          "data": {
            "issuer": "${your account name}",
            "maximum_supply": "9000000000.0000 CUR",
            "can_freeze": 1,
            "can_recall": 1,
            "can_whitelist": 1
          },
          "hex_data": "000000000090284d00a007c2da5100000443555200000000010101"
        },
        "console": "create\n",
        "region_id": 0,
        "cycle_index": 1,
        "data_access": [{
            "type": "write",
            "code": "${your account name}",
            "scope": "........edeo3",
            "sequence": 0
          },{
            "type": "write",
            "code": "${your account name}",
            "scope": "${your account name}",
            "sequence": 0
          }
        ]
      }
    ],
    "deferred_transaction_requests": []
  }
}
```

接下来：

```bash
./cleos push action ${your account name} issue '{"to":"${your account name}","quantity":"1000.0000 CUR","memo":""}' --permission ${your account name}@active
```

返回大段JSON即为成功，接下来我们查看一下当前的情况了，相信你能看的懂。

```bash
$ ./cleos get table ${your account name} CUR stat
{
  "rows": [{
      "supply": "1001.0000 CUR",
      "max_supply": "9000000000.0000 CUR",
      "issuer": "${your account name}",
      "can_freeze": 1,
      "can_recall": 1,
      "can_whitelist": 1,
      "is_frozen": 0,
      "enforce_whitelist": 0
    }
  ],
  "more": false
}
```

<a name="pushamessage"></a>
### 现在我们来交易一下~

项目提供的币合约，默认包含了 `transfer` 的动作。

等下我们传送的内容大概类似`'{"from""${your account name},"to":"eosio","quantity":"20.0000 CUR","memo":"any string"}'`，也就是给 `eosio` 转点钱...

```bash
$ ./cleos push action ${your account name} transfer '{"from":"${your account name}","to":"eosio","quantity":"1000.0000 CUR","memo":"my first transfer"}' --permission ${your account name}@active
```

这里的 `quantity`，请比之前的 `1001` 小，接下来我们看结果：

```json
{
  "transaction_id": "b036e58c3db741550fa19a4e59993070ef4c8d36b494a066843acd9ac1d0f0a6",
  "processed": {
    "status": "executed",
    "id": "b036e58c3db741550fa19a4e59993070ef4c8d36b494a066843acd9ac1d0f0a6",
    "action_traces": [{
        "receiver": "${your account name}",
        "act": {
          "account": "${your account name}",
          "name": "transfer",
          "authorization": [{
              "actor": "${your account name}",
              "permission": "active"
            }
          ],
          "data": {
            "from": "${your account name}",
            "to": "eosio",
            "quantity": "1000.0000 CUR",
            "memo": "my first transfer"
          },
          "hex_data": "000000000090284d0000000000ea305580969800000000000443555200000000116d79206669727374207472616e73666572"
        },
        "console": "transfer\n",
        "region_id": 0,
        "cycle_index": 1,
        "data_access": [{
            "type": "write",
            "code": "${your account name}",
            "scope": "${your account name}",
            "sequence": 2
          },{
            "type": "write",
            "code": "${your account name}",
            "scope": "eosio",
            "sequence": 0
          },{
            "type": "read",
            "code": "${your account name}",
            "scope": "........edeo3",
            "sequence": 2
          }
        ]
      },{
        "receiver": "eosio",
        "act": {
          "account": "${your account name}",
          "name": "transfer",
          "authorization": [{
              "actor": "${your account name}",
              "permission": "active"
            }
          ],
          "data": {
            "from": "${your account name}",
            "to": "eosio",
            "quantity": "1000.0000 CUR",
            "memo": "my first transfer"
          },
          "hex_data": "000000000090284d0000000000ea305580969800000000000443555200000000116d79206669727374207472616e73666572"
        },
        "console": "",
        "region_id": 0,
        "cycle_index": 1,
        "data_access": []
      }
    ],
    "deferred_transaction_requests": []
  }
}
```

此时我们进行一下确认，使用查询账号信息的方法：

```bash
$ ./cleos get table ${your contract name} eosio accounts
# 这里的 eosio 可以为其他想查询的账户名
{
  "rows": [{
      "balance": "1000.0000 CUR",
      "frozen": 0,
      "whitelist": 1
    }
  ],
  "more": false
}
$ ./cleos get table ${your contract name} ${your account name} accounts
# 查询您的合约下，账户名称的账户信息
{
  "rows": [{
      "balance": "1.0000 CUR",
      "frozen": 0,
      "whitelist": 1
    }
  ],
  "more": false
}
```

<a name="readingcontract"></a>
### Reading sample "currency" contract balance

上边已经告诉你啦~，我们转移了1000到 `eosio` 下。

<a name="localtestnet"></a>
## 运行本地多节点测试

```bash
cd ~/eos/build
cp ../genesis.json ./
./programs/eosio-launcher/eosio-launcher -p2 --skip-signature
```

将会生成两个数据文件夹：`tn_data_00` and `tn_data_01`。

将会看到类似如下的输出：

```bash
spawning child, programs/nodeos/nodeos --skip-transaction-signatures --data-dir tn_data_0
spawning child, programs/nodeos/nodeos --skip-transaction-signatures --data-dir tn_data_1
```

确认节点运行：
```bash
~/eos/build/programs/cleos
./cleos -p 8888 get info
./cleos -p 8889 get info
```

[获取更多详细指引](https://github.com/EOSIO/eos/blob/master/testnet.md)

<a name="publictestnet"></a>
## 本地节点接入公共测试环境

本节等待更新中… 目前官方英文文档错误。

<a name="doxygen"></a>

特别说明：文档中错误希望指出，操作过程可能与英文版 README.md 中不符，但是可以正常在本地环境中进行测试。
