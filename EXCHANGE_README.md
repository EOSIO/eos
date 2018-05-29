Exchange Deposit & Withdraw Documentation
-----------------------------------------

This document is targeted toward exchanges who wish to automate deposit
and withdraw of standard-conforming EOSIO token contracts. The blockchain's
native token conforms to the standard. 


Configuring Nodeos
------------------
This tutorial uses the `cleos` commandline tool to query a local `nodeos` server
which should be connected to an eosio blockchain. `nodeos` will need to be configured
with the following plugins:

  1. eosio::wallet_api_plugin 
  2. eosio::history_api_plugin
  3. eosio::chain_api_plugin

By default the history plugin will log the history of all accounts, but this is not
the recomended configuration as it will consume tens of gigabytes of RAM in the
medium term.  For a more optimized memory footprint you should configure the history
plugin to only log activity relevant to your account(s).  This can be achieved with
the following config param placed in your config.ini or passed on the commandline.

```
  $ nodeos --filter_on_accounts youraccount
```

Replaying the Blockchain
------------------------

If you have already synced the blockchain without the history plugin, then you may need to
replay the blockchain to pickup any historical activity.

```
 $ nodeos --replay --filter_on_accounts youraccount 
```

You only need to replay once, subsequent runs of nodeos should not use the replay flag or
your startup times will be unnecessiarlly long. 


Accepting Deposits
-----------
When designing this tutorial we assume that an exchange will poll `nodeos` for incoming
transactions and will want to know when a transfer is considered irreversible or final. 

With eosio based chains, finality of a transaction occurs once 2/3+1 of block produers have
either directly or indirectly confirmed the block. This could take from less than a second to
a couple of minutes, but either way nodeos will keep you posted on the status.

## Initial Condition
```
./cleos get currency balance eosio.token scott EOS
900.0000 EOS
```

We will now deposit some funds to exchange:

```
./cleos transfer scott exchange "1.0000 EOS"
executed transaction: 5ec797175dd24612acd8fc5a8685fa44caa8646cec0a87b12568db22a3df02fb  256 bytes  8k cycles
#   eosio.token <= eosio.token::transfer        {"from":"scott","to":"exchange","quantity":"1.0000 EOS","memo":""}
>> transfer
#         scott <= eosio.token::transfer        {"from":"scott","to":"exchange","quantity":"1.0000 EOS","memo":""}
#      exchange <= eosio.token::transfer        {"from":"scott","to":"exchange","quantity":"1.0000 EOS","memo":""}
warning: transaction executed locally, but may not be confirmed by the network yet
```

This output indicates that the action "eosio.token::transfer" was delivered to 3 accounts/contracts, (eosio.token, scott, and exchange). 
The eosio token standard requires that both the sender and receiver account/contract be notified of all transfer actions so those
accounts can run custom logic.  At this time neither `scott` nor `exchange` has any contact set, but the transaction log
still notes that they were notified.  


## Polling Account History 
The account history consists of all actions which were either authorized by the account or received by the account. Since the
exchange received the `eosio.token::transfer` action it is listed in the history. If you are using the console confirmed and
irreversible transactions are printed in "green" while unconfirmed transactions are printed in "yellow". Without color you
can tell whether a transaction is confirmed or not by the first character, '#' for irreversible and '?' for potentially reversable.

```
./cleos get actions exchange
#  seq  when                              contract::action => receiver      trx id...   args
================================================================================================================
#    0   2018-04-29T01:09:45.000     eosio.token::transfer => exchange      5ec79717... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
```

Do a few more transfers:

```
./cleos get actions exchange
#  seq  when                              contract::action => receiver      trx id...   args
================================================================================================================
#    0   2018-04-29T01:09:45.000     eosio.token::transfer => exchange      5ec79717... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
#    1   2018-04-29T01:16:25.000     eosio.token::transfer => exchange      2269828c... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
?    2   2018-04-29T01:19:54.000     eosio.token::transfer => exchange      213f3797... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
```

The last transfer is still pending, waiting on irreversibility. 


The "seq" column represents the index of actions for your specific account, it will always increment as new relevant actions are added.

The `cleos get actions` command allows you some control over which actions are fetched, you can view the help for this command with `-h` 

```
./cleos get actions -h
Usage: ./cleos get actions [OPTIONS] account_name [pos] [offset]

Positionals:
  account_name TEXT           name of account to query on
  pos INT                     sequence number of action for this account, -1 for last
  offset INT                  get actions [pos,pos+offset] for positive offset or [pos-offset,pos) for negative offset

  Options:
    -j,--json                   print full json
    --full                      don't truncate action json
    --pretty                    pretty print full action json
    --console                   print console output generated by action
```

To get only the last action you would do the following...

```
./cleos get actions exchange -1 -1
#  seq  when                              contract::action => receiver      trx id...   args
================================================================================================================
#    2   2018-04-29T01:19:54.000     eosio.token::transfer => exchange      213f3797... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
```

This says go to the last sequence number (indicated by pos = -1) and then fetch "1" item prior to it (offset = -1). This should
return sequence in the range [3-1,3) or [2,3) which is only row 2.  In this case "-1" position means "one past the last sequence" and
operates like and end iterator from c++ containers.

### Fetching only "New" Actions

Since we presume your exchange is running a polling micro-service, it will want to fetch the "next unprocessed deposit". In this case the
microservice will need to track the seq number of the "last processed seq".  For the sake of this example, we will assume that
"seq 0" has been processed and that we want to fetch "seq 1" if any.

We pass pos=1 and offset=0 to get the range [1,1+0] or [1,1].
```
./cleos get actions exchange 1 0
#  seq  when                              contract::action => receiver      trx id...   args
================================================================================================================
#    1   2018-04-29T01:16:25.000     eosio.token::transfer => exchange      2269828c... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
```

We can call this in a loop procesing each confirmed action (those starting with #) until we either run out of items or
we find an unconfirmed action (starting with ?).

```
./cleos get actions exchange 3 0
#  seq  when                              contract::action => receiver      trx id...   args
================================================================================================================
```

### Machine Readable Account History (JSON)

So far this tutorial has focused on using `cleos` to fetch and display the history, but cleos is merely a light-weight
wrapper around a json-rpc interface.  `cleos` can dump the raw json returned from the json-rpc request or you can make
your own json-rpc request.

Here is the JSON returned when querying sequence 2.
```
./cleos get actions exchange 2 0 -j
{
  "actions": [{
      "global_action_seq": 32856,
      "account_action_seq": 2,
      "block_num": 32759,
      "block_time": "2018-04-29T01:19:54.000",
      "action_trace": {
        "receipt": {
          "receiver": "exchange",
          "act_digest": "00686ff415fe97951a942889dbaed2b880043e3ae6ac2d5579318bbb2d30060f",
          "global_sequence": 32856,
          "recv_sequence": 3,
          "auth_sequence": [[
              "scott",
              43
            ]
          ]
        },
        "act": {
          "account": "eosio.token",
          "name": "transfer",
          "authorization": [{
              "actor": "scott",
              "permission": "active"
            }
          ],
          "data": {
            "from": "scott",
            "to": "exchange",
            "quantity": "1.0000 EOS",
            "memo": ""
          },
          "hex_data": "00000000809c29c20000008a4dd35057102700000000000004454f530000000000"
        },
        "elapsed": 52,
        "cpu_usage": 1000,
        "console": "",
        "total_inline_cpu_usage": 1000,
        "trx_id": "213f37972498cbae5abf6bcb5aec82e09967df7f04cf90f67b7d63a6bb871d58",
        "inline_traces": []
      }
    }
  ],
  "last_irreversible_block": 35062
}
```

Given this JSON, an action is irreversible (final) if `"block_num" < "last_irreversible_block"`.  

You can identify irreversible deposits by the following:

```
    actions[0].action_trace.act.account == "eosio.token" &&
    actions[0].action_trace.act.name == "transfer" &&
    actions[0].action_trace.act.data.quantity == "X.0000 EOS" &&
    actions[0].action_trace.to == "exchange" && 
    actions[0].action_trace.memo == "KEY TO IDENTIFY INTERNAL ACCOUNT" && 
    actions[0].action_trace.receipt.receiver == "exchange"  &&
    actions[0].block_num < last_irreversible_block
```

In practice you should give your customers a "memo" that identifies which of your internal accounts you should
credit with the deposit.

## WARNING

It is critical that you validate all of the conditions above, including the token symbol name. Users can create
other contracts with "transfer" actions that "notify" your account. If you do not validate all of the above properties
then you may process "false deposits".  

```
    actions[0].action_trace.act.account == "eosio.token" &&
    actions[0].action_trace.receipt.receiver == "exchange" 
```

### Validating Balance

Now that we have received 3 deposits we should see that the exchange has a balance of 3.0000 EOS.

```
./cleos get currency balance eosio.token exchange EOS
3.0000 EOS
```

# Processing Withdraws

(note, while generating this tutorial scott deposited another 1.0000 EOS (seq 3) for total exchange balance of 4.0000 EOS.)

When a user requests a withdraw from your exchange they will need to provide you with their eosio account name and
the amount to be withdrawn.  You can then run the cleos command which will interact with the "unlocked" wallet 
running on `nodeos` which should only enable localhost connections. More advanced usage would have a separate
key-server (`keos`), but that will be covered later.

Lets assume scott wants to withdraw `1.0000 EOS`:
```
./cleos transfer exchange scott  "1.0000 EOS"
executed transaction: 93e785202e7502bb1383ad10e786cc20f7dd738d3fd3da38712b3fb38fb9af26  256 bytes  8k cycles
#   eosio.token <= eosio.token::transfer        {"from":"exchange","to":"scott","quantity":"1.0000 EOS","memo":""}
>> transfer
#      exchange <= eosio.token::transfer        {"from":"exchange","to":"scott","quantity":"1.0000 EOS","memo":""}
#         scott <= eosio.token::transfer        {"from":"exchange","to":"scott","quantity":"1.0000 EOS","memo":""}
warning: transaction executed locally, but may not be confirmed by the network yet
```

At this stage your local `nodeos` client accepted the transaction and likely broadcast it to the broader network. 

Now we can get the history and see that there are "3" new actions listed all with trx id `93e78520...` which is what
our transfer command returned to us. Because `exchange` authorized the transaction it is informed of all accounts which
processed and accepted the 'transfer'.  In this case the 'eosio.token' contract processed it and updated balances, the
sender ('exchange') processed it and so did the receiver ('scott') and all 3 contracts/accounts approved it and/or performed
state transitions based upon the action.

```
./cleos get actions exchange -1 -8
#  seq  when                              contract::action => receiver      trx id...   args
================================================================================================================
#    0   2018-04-29T01:09:45.000     eosio.token::transfer => exchange      5ec79717... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
#    1   2018-04-29T01:16:25.000     eosio.token::transfer => exchange      2269828c... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
#    2   2018-04-29T01:19:54.000     eosio.token::transfer => exchange      213f3797... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
#    3   2018-04-29T01:53:57.000     eosio.token::transfer => exchange      8b7766ac... {"from":"scott","to":"exchange","quantity":"1.0000 EOS","mem...
#    4   2018-04-29T01:54:17.500     eosio.token::transfer => eosio.token   93e78520... {"from":"exchange","to":"scott","quantity":"1.0000 EOS","mem...
#    5   2018-04-29T01:54:17.500     eosio.token::transfer => exchange      93e78520... {"from":"exchange","to":"scott","quantity":"1.0000 EOS","mem...
#    6   2018-04-29T01:54:17.500     eosio.token::transfer => scott         93e78520... {"from":"exchange","to":"scott","quantity":"1.0000 EOS","mem...
```

By processing the history we can also be informed when our transaction was confirmed. In practice it may be useful to embed an exchange-specify memo
on the withdraw request which you can use to map to your private database state which tracks the withdraw process *or* you could simply use the
transaction ID.  When your account history microservice comes across seq 5 and sees it is irreversible it can then mark your withdaw as complete.

### Handling Errors

Sometimes network issues may cause a transaction to fail and never be included in a block. Your internal database will need to know when this has happend
so that it can inform the user and/or try again. If you do not get an immediate error when you submit your local transfer, then you must wait for
the transaction to expire. Every transaction has an "expiration" after which the transaction can never be applied. Once the last irreversible block has
moved past the expiration time you can safely mark your attempted withdaw as failed and not worry about it "floating around the ether" to be applied
when you least expect.

By default cleos sets an expiration window of just 2 minutes.  This is long enough to allow all 21 producers an opportunity to include the transaction.

```
 ./cleos transfer exchange scott  "1.0000 EOS" -j -d
{
  "expiration": "2018-04-29T01:58:12",
  "ref_block_num": 37282,
  "ref_block_prefix": 351570603,
  "max_net_usage_words": 0,
  "max_kcpu_usage": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  ...

```

Your microservice can query the last irreversible block number and the head block time using cleos. 
```
./cleos get info
{
  "server_version": "0812f84d",
  "head_block_num": 39313,
  "last_irreversible_block_num": 39298,
  "last_irreversible_block_id": "000099823bfc4f0b936d8e48c70fc3f1619eb8d21989d160a9fe23655f1f5c79",
  "head_block_id": "000099912473a7a3699ad682f731d1874ebddcf4b60eff79f8e6e4216077278d",
  "head_block_time": "2018-04-29T02:14:31",
  "head_block_producer": "producer2"
}
```

### Exchange Security 

This tutorial shows the minimal viable deposit/withdraw handlers and assumes a single wallet which contains all keys necessary to
authorize deposits and withdaws. A security-focused exchange would take the following additional steps:

1. keep vast majority of funds in a time-delayed, multi-sig controlled account
2. use multi-sig on the hot wallet with several independent processes/servers double-checking all withdraws 
3. deploy a custom contract that only allows withdraws to KYC'd accounts and require multi-sig to white-list accounts
4. deploy a custom contract that only accepts deposits of known tokens from KYC'd accounts
5. deploy a custom contract that enforces a mandatory 24 hour waiting period for all withdraws
6. utilize hardware wallets for all signing, even automated withdraw

Customer's want immediate withdraws, but they also want the exchange to be protected. The blockchain-enforced 24 hour period 
lets the customer know the money is "on the way" while also informing potential-hackers that the exchange has 24 hours to
respond to unauthorized access. Furthermore, if the exchange emails/text messages users upon start of withdraw, users have
24 hours to contact the exchange and fix any unauthorized access to their individual account.

Information on how to utilize these more advanced techniques will be available in a future document.












