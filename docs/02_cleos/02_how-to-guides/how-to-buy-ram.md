## Goal

Acquire RAM for contract deployment, database tables, and other blockchain resources

## Before you begin
Before you follow the step, make sure you fulfill the following items: 

* You have an EOSIO account

* Ensure the reference system contracts from [`eosio.contracts`](https://github.com/EOSIO/eosio.contracts) repository is deployed and used to manage system resources

* You have sufficient [token allocated](how-to-transfer-an-eosio.token-token.md) to your account

* Install the currently supported version of `cleos`

[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos tool. 

* Unlock your wallet using the `cleos wallet unlock` command


## Steps

Buys RAM in value of 0.1 SYS tokens for account `alice`:

```sh
cleos system buyram alice alice "0.1 SYS" -p alice@active
```

**Example Output**
```sh
# cleos system buyram alice alice "0.1 SYS" -p alice@active
executed transaction: aa243c30571a5ecc8458cb971fa366e763682d89b636fe9dbe7d28327d1cc4e9  128 bytes  283 us
#         eosio <= eosio::buyram                {"payer":"alice","receiver":"alice","quant":"0.1000 SYS"}
#   eosio.token <= eosio.token::transfer        {"from":"alice","to":"eosio.ram","quantity":"0.0995 SYS","memo":"buy ram"}
#   eosio.token <= eosio.token::transfer        {"from":"alice","to":"eosio.ramfee","quantity":"0.0005 SYS","memo":"ram fee"}
#         alice <= eosio.token::transfer        {"from":"alice","to":"eosio.ram","quantity":"0.0995 SYS","memo":"buy ram"}
#     eosio.ram <= eosio.token::transfer        {"from":"alice","to":"eosio.ram","quantity":"0.0995 SYS","memo":"buy ram"}
#         alice <= eosio.token::transfer        {"from":"alice","to":"eosio.ramfee","quantity":"0.0005 SYS","memo":"ram fee"}
#  eosio.ramfee <= eosio.token::transfer        {"from":"alice","to":"eosio.ramfee","quantity":"0.0005 SYS","memo":"ram fee"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

