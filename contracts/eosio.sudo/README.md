# eosio.sudo

## 1. Actions:
The naming convention is codeaccount::actionname followed by a list of parameters.

Execute a transaction while bypassing regular authorization checks (requires authorization of eosio.sudo which needs to be a privileged account).

### eosio.sudo::exec    executer trx
   - **executer** account executing the transaction
   - **trx** transaction to execute

   Deferred transaction RAM usage is billed to 'executer'


## 2. Installing the eosio.sudo contract

The eosio.sudo contract needs to be installed on a privileged account to function. It is recommended to use the account `eosio.sudo`.

First, the account `eosio.sudo` needs to be created. Since it has the restricted `eosio.` prefix, only a privileged account can create this account. So this guide will use the `eosio` account to create the `eosio.sudo` account. On typical live blockchain configurations, the `eosio` account can only be controlled by a supermajority of the current active block producers. So, this guide will use the `eosio.msig` contract to help coordinate the approvals of the proposed transaction that creates the `eosio.sudo` account.

The `eosio.sudo` account also needs to have sufficient RAM to host the contract and sufficient CPU and network bandwidth to deploy the contract. This means that the creator of the account (`eosio`) needs to gift sufficient RAM to the new account and delegate (preferably with transfer) sufficient bandwidth to the new account. To pull this off the `eosio` account needs to have enough of the core system token (the `SYS` token will be used within this guide) in its liquid balance. So prior to continuing with the next steps of this guide, the active block producers of the chain who are coordinating this process need to ensure that a sufficient amount of core system tokens that they are authorized to spend is placed in the liquid balance of the `eosio` account.

This guide will be using cleos to carry out the process.

### 2.1 Create the eosio.sudo account

#### 2.1.1 Generate the transaction to create the eosio.sudo account

The transaction to create the `eosio.sudo` account will need to be proposed to get the necessary approvals from active block producers before executing it. This transaction needs to first be generated and stored as JSON into a file so that it can be used in the cleos command to propose the transaction to the eosio.msig contract.

A simple way to generate a transaction to create a new account is to use the `cleos system newaccount`. However, that sub-command currently only accepts a single public key as the owner and active authority of the new account. However, the owner and active authorities of the new account should only be satisfied by the `active` permission of `eosio`. One option is to create the new account with the some newly generated key, and then later update the authorities of the new account using `cleos set account permission`. This guide will take an alternative approach which atomically creates the new account in its proper configuration.

Three unsigned transactions will be generated using cleos and then the actions within those transactions will be appropriately stitched together into a single transaction which will later be proposed using the eosio.msig contract.

First, generate a transaction to capture the necessary actions involved in creating a new account:
```
$ cleos system newaccount -s -j -d --transfer --stake-net "1.000 SYS" --stake-cpu "1.000 SYS" --buy-ram-kbytes 50 eosio eosio.sudo EOS8MMUW11TAdTDxqdSwSqJodefSoZbFhcprndomgLi9MeR2o8MT4 > generated_account_creation_trx.json
726964ms thread-0   main.cpp:429                  create_action        ] result: {"binargs":"0000000000ea305500004d1a03ea305500c80000"} arg: {"code":"eosio","action":"buyrambytes","args":{"payer":"eosio","receiver":"eosio.sudo","bytes":51200}}
726967ms thread-0   main.cpp:429                  create_action        ] result: {"binargs":"0000000000ea305500004d1a03ea3055102700000000000004535953000000001027000000000000045359530000000001"} arg: {"code":"eosio","action":"delegatebw","args":{"from":"eosio","receiver":"eosio.sudo","stake_net_quantity":"1.0000 SYS","stake_cpu_quantity":"1.0000 SYS","transfer":true}}
$ cat generated_account_creation_trx.json
{
  "expiration": "2018-06-29T17:11:36",
  "ref_block_num": 16349,
  "ref_block_prefix": 3248946195,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "newaccount",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea305501000000010003c8162ea04fed738bfd5470527fd1ae7454c2e9ad1acbadec9f9e35bab2f33c660100000001000000010003c8162ea04fed738bfd5470527fd1ae7454c2e9ad1acbadec9f9e35bab2f33c6601000000"
    },{
      "account": "eosio",
      "name": "buyrambytes",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea305500c80000"
    },{
      "account": "eosio",
      "name": "delegatebw",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea3055102700000000000004535953000000001027000000000000045359530000000001"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```
Adjust the amount of delegated tokens and the amount of RAM bytes to gift as necessary.
The actual public key used is not important since that data is only encoded into the `eosio::newaccount` action which will be replaced soon anyway.

Second, create a file (e.g. newaccount_payload.json) with the JSON payload for the real `eosio::newaccount` action. It should look like:
```
$ cat newaccount_payload.json
{
   "creator": "eosio",
   "name": "eosio.sudo",
   "owner": {
      "threshold": 1,
      "keys": [],
      "accounts": [{
         "permission": {"actor": "eosio", "permission": "active"},
         "weight": 1
      }],
      "waits": []
   },
   "active": {
      "threshold": 1,
      "keys": [],
      "accounts": [{
         "permission": {"actor": "eosio", "permission": "active"},
         "weight": 1
      }],
      "waits": []
   }
}
```

Third, generate a transaction containing the actual `eosio::newaccount` action that will be used in the final transaction:
```
$ cleos push action -s -j -d eosio newaccount newaccount_payload.json -p eosio > generated_newaccount_trx.json
$ cat generated_newaccount_trx.json
{
  "expiration": "2018-06-29T17:11:36",
  "ref_block_num": 16349,
  "ref_block_prefix": 3248946195,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "newaccount",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea30550100000000010000000000ea305500000000a8ed32320100000100000000010000000000ea305500000000a8ed3232010000"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

Fourth, generate a transaction containing the `eosio::setpriv` action which will make the `eosio.sudo` account privileged:
```
$ cleos push action -s -j -d eosio setpriv '{"account": "eosio.sudo", "is_priv": 1}' -p eosio > generated_setpriv_trx.json
$ cat generated_setpriv_trx.json
{
  "expiration": "2018-06-29T17:11:36",
  "ref_block_num": 16349,
  "ref_block_prefix": 3248946195,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "setpriv",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "00004d1a03ea305501"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

Next, the action JSONs of the previously generated transactions will be used to construct a unified transaction which will eventually be proposed with the eosio.msig contract. A good way to get started is to make a copy of the generated_newaccount_trx.json file (call the copied file create_sudo_account_trx.json) and edit the first three fields so it looks something like the following:
```
$ cat create_sudo_account_trx.json
{
  "expiration": "2018-07-06T12:00:00",
  "ref_block_num": 0,
  "ref_block_prefix": 0,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "newaccount",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea30550100000000010000000000ea305500000000a8ed32320100000100000000010000000000ea305500000000a8ed3232010000"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

The `ref_block_num` and `ref_block_prefix` values were set to 0. The proposed transaction does not need to have a valid TaPoS reference block because it will be reset anyway when scheduled as a deferred transaction during the `eosio.msig::exec` action. The `expiration` field, which was the only other field that was changed, will also be reset when the proposed transaction is scheduled as a deferred transaction during `eosio.msig::exec`. However, this field actually does matter during the propose-approve-exec lifecycle of the proposed transaction. If the present time passes the time in the `expiration` field of the proposed transaction, it will not be possible to execute the proposed transaction even if all necessary approvals are gathered. Therefore, it is important to set the expiration time to some point well enough in the future to give all necessary approvers enough time to review and approve the proposed transaction, but it is otherwise arbitrary. Generally, for reviewing/validation purposes it is important that all potential approvers of the transaction (i.e. the block producers) choose the exact same `expiration` time so that there is not any discrepancy in bytes of the serialized transaction if it was to later be included in payload data of some other action.

Then, all but the first action JSON object of generated_account_creation_trx.json should be appended to the `actions` array of create_sudo_account_trx.json, and then the single action JSON object of generated_setpriv_trx.json should be appended to the `actions` array of create_sudo_account_trx.json. The final result is a create_sudo_account_trx.json file that looks like the following:
```
$ cat create_sudo_account_trx.json
{
  "expiration": "2018-07-06T12:00:00",
  "ref_block_num": 0,
  "ref_block_prefix": 0,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "newaccount",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea30550100000000010000000000ea305500000000a8ed32320100000100000000010000000000ea305500000000a8ed3232010000"
    },{
      "account": "eosio",
      "name": "buyrambytes",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea305500c80000"
    },{
      "account": "eosio",
      "name": "delegatebw",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500004d1a03ea3055102700000000000004535953000000001027000000000000045359530000000001"
    },{
      "account": "eosio",
      "name": "setpriv",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "00004d1a03ea305501"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

The transaction in create_sudo_account_trx.json is now ready to be proposed.

It will be useful to have a JSON of the active permissions of each of the active block producers for later when proposing transactions using the eosio.msig contract.

This guide will assume that there are 21 active block producers on the chain with account names: `blkproducera`, `blkproducerb`, ..., `blkproduceru`.

In that case, create a file producer_permissions.json with the content shown in the command below:
```
$ cat producer_permissions.json
[
   {"actor": "blkproducera", "permission": "active"},
   {"actor": "blkproducerb", "permission": "active"},
   {"actor": "blkproducerc", "permission": "active"},
   {"actor": "blkproducerd", "permission": "active"},
   {"actor": "blkproducere", "permission": "active"},
   {"actor": "blkproducerf", "permission": "active"},
   {"actor": "blkproducerg", "permission": "active"},
   {"actor": "blkproducerh", "permission": "active"},
   {"actor": "blkproduceri", "permission": "active"},
   {"actor": "blkproducerj", "permission": "active"},
   {"actor": "blkproducerk", "permission": "active"},
   {"actor": "blkproducerl", "permission": "active"},
   {"actor": "blkproducerm", "permission": "active"},
   {"actor": "blkproducern", "permission": "active"},
   {"actor": "blkproducero", "permission": "active"},
   {"actor": "blkproducerp", "permission": "active"},
   {"actor": "blkproducerq", "permission": "active"},
   {"actor": "blkproducerr", "permission": "active"},
   {"actor": "blkproducers", "permission": "active"},
   {"actor": "blkproducert", "permission": "active"},
   {"actor": "blkproduceru", "permission": "active"}
]
```

#### 2.1.2 Propose the transaction to create the eosio.sudo account

Only one of the potential approvers will need to propose the transaction that was created in the previous sub-section. All the other approvers should still follow the steps in the previous sub-section to generate the same create_sudo_account_trx.json file as all the other approvers. They will need this to compare to the actual proposed transaction prior to approving.

The approvers are typically going to be the active block producers of the chain, so it makes sense that one of the block producers is elected as the leader to propose the actual transaction. Note that this lead block producer will need to incur the temporary RAM cost of proposing the transaction, but they will get the RAM back when the proposal has executed or has been canceled (which only the proposer can do prior to expiration).

The guide will assume that `blkproducera` was chosen as the lead block producer to propose the transaction.

The lead block producer (`blkproducera`) should propose the transaction stored in create_sudo_account_trx.json:
```
$ cleos multisig propose_trx createsudo producer_permissions.json create_sudo_account_trx.json blkproducera
executed transaction: bf6aaa06b40e2a35491525cb11431efd2b5ac94e4a7a9c693c5bf0cfed942393  744 bytes  772 us
#    eosio.msig <= eosio.msig::propose          {"proposer":"blkproducera","proposal_name":"createsudo","requested":[{"actor":"blkproducera","permis...
warning: transaction executed locally, but may not be confirmed by the network yet
```

#### 2.1.3 Review and approve the transaction to create the eosio.sudo account

Each of the potential approvers of the proposed transaction (i.e. the active block producers) should first review the proposed transaction to make sure they are not approving anything that they do not agree to.

The proposed transaction can be reviewed using the `cleos multisig review` command:
```
$ cleos multisig review blkproducera createsudo > create_sudo_account_trx_to_review.json
$ head -n 30 create_sudo_account_trx_to_review.json
{
  "proposal_name": "createsudo",
  "packed_transaction": "c0593f5b00000000000000000000040000000000ea305500409e9a2264b89a010000000000ea305500000000a8ed3232420000000000ea305500004d1a03ea30550100000000010000000000ea305500000000a8ed32320100000100000000010000000000ea305500000000a8ed32320100000000000000ea305500b0cafe4873bd3e010000000000ea305500000000a8ed3232140000000000ea305500004d1a03ea3055002800000000000000ea305500003f2a1ba6a24a010000000000ea305500000000a8ed3232310000000000ea305500004d1a03ea30551027000000000000045359530000000010270000000000000453595300000000010000000000ea305500000060bb5bb3c2010000000000ea305500000000a8ed32320900004d1a03ea30550100",
  "transaction": {
    "expiration": "2018-07-06T12:00:00",
    "ref_block_num": 0,
    "ref_block_prefix": 0,
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "actions": [{
        "account": "eosio",
        "name": "newaccount",
        "authorization": [{
            "actor": "eosio",
            "permission": "active"
          }
        ],
        "data": {
          "creator": "eosio",
          "name": "eosio.sudo",
          "owner": {
            "threshold": 1,
            "keys": [],
            "accounts": [{
                "permission": {
                  "actor": "eosio",
                  "permission": "active"
                },
```

The approvers should go through the full human-readable transaction output and make sure everything looks fine. But they can also use tools to automatically compare the proposed transaction to the one they generated to make sure there are absolutely no differences:
```
$ cleos multisig propose_trx -j -s -d createsudo '[]' create_sudo_account_trx.json blkproducera | grep '"data":' | sed 's/^[ \t]*"data":[ \t]*//;s/[",]//g' | cut -c 35- > expected_create_sudo_trx_serialized.hex
$ cat expected_create_sudo_trx_serialized.hex
c0593f5b00000000000000000000040000000000ea305500409e9a2264b89a010000000000ea305500000000a8ed3232420000000000ea305500004d1a03ea30550100000000010000000000ea305500000000a8ed32320100000100000000010000000000ea305500000000a8ed32320100000000000000ea305500b0cafe4873bd3e010000000000ea305500000000a8ed3232140000000000ea305500004d1a03ea3055002800000000000000ea305500003f2a1ba6a24a010000000000ea305500000000a8ed3232310000000000ea305500004d1a03ea30551027000000000000045359530000000010270000000000000453595300000000010000000000ea305500000060bb5bb3c2010000000000ea305500000000a8ed32320900004d1a03ea30550100
$ cat create_sudo_account_trx_to_review.json | grep '"packed_transaction":' | sed 's/^[ \t]*"packed_transaction":[ \t]*//;s/[",]//g' > proposed_create_sudo_trx_serialized.hex
$ cat proposed_create_sudo_trx_serialized.hex
c0593f5b00000000000000000000040000000000ea305500409e9a2264b89a010000000000ea305500000000a8ed3232420000000000ea305500004d1a03ea30550100000000010000000000ea305500000000a8ed32320100000100000000010000000000ea305500000000a8ed32320100000000000000ea305500b0cafe4873bd3e010000000000ea305500000000a8ed3232140000000000ea305500004d1a03ea3055002800000000000000ea305500003f2a1ba6a24a010000000000ea305500000000a8ed3232310000000000ea305500004d1a03ea30551027000000000000045359530000000010270000000000000453595300000000010000000000ea305500000060bb5bb3c2010000000000ea305500000000a8ed32320900004d1a03ea30550100
$ diff expected_create_sudo_trx_serialized.hex proposed_create_sudo_trx_serialized.hex
```

When an approver (e.g. `blkproducerb`) is satisfied with the proposed transaction, they can simply approve it:
```
$ cleos multisig approve blkproducera createsudo '{"actor": "blkproducerb", "permission": "active"}' -p blkproducerb
executed transaction: 03a907e2a3192aac0cd040c73db8273c9da7696dc7960de22b1a479ae5ee9f23  128 bytes  472 us
#    eosio.msig <= eosio.msig::approve          {"proposer":"blkproducera","proposal_name":"createsudo","level":{"actor":"blkproducerb","permission"...
warning: transaction executed locally, but may not be confirmed by the network yet
```

#### 2.1.4 Execute the transaction to create the eosio.sudo account

When the necessary approvals are collected (in this example, with 21 block producers, at least 15 of their approvals were required), anyone can push the `eosio.msig::exec` action which executes the approved transaction. It makes a lot of sense for the lead block producer who proposed the transaction to also execute it (this will incur another temporary RAM cost for the deferred transaction that is generated by the eosio.msig contract).

```
$ cleos multisig exec blkproducera createsudo blkproducera
executed transaction: 7ecc183b99915cc411f96dde7c35c3fe0df6e732507f272af3a039b706482e5a  160 bytes  850 us
#    eosio.msig <= eosio.msig::exec             {"proposer":"blkproducera","proposal_name":"createsudo","executer":"blkproducera"}
warning: transaction executed locally, but may not be confirmed by the network yet
```

Anyone can now verify that the `eosio.sudo` was created:
```
$ cleos get account eosio.sudo
privileged: true
permissions:
     owner     1:    1 eosio@active,
        active     1:    1 eosio@active,
memory:
     quota:     49.74 KiB    used:     3.33 KiB  

net bandwidth:
     staked:          1.0000 SYS           (total stake delegated from account to self)
     delegated:       0.0000 SYS           (total staked delegated to account from others)
     used:                 0 bytes
     available:        2.304 MiB  
     limit:            2.304 MiB  

cpu bandwidth:
     staked:          1.0000 SYS           (total stake delegated from account to self)
     delegated:       0.0000 SYS           (total staked delegated to account from others)
     used:                 0 us   
     available:        460.8 ms   
     limit:            460.8 ms   

producers:     <not voted>

```

### 2.2 Deploy the eosio.sudo contract

#### 2.2.1  Generate the transaction to deploy the eosio.sudo contract

The transaction to deploy the contract to the `eosio.sudo` account will need to be proposed to get the necessary approvals from active block producers before executing it. This transaction needs to first be generated and stored as JSON into a file so that it can be used in the cleos command to propose the transaction to the eosio.msig contract.

The easy way to generate this transaction is using cleos:
```
$ cleos set contract -s -j -d eosio.sudo contracts/eosio.sudo/ > deploy_sudo_contract_trx.json
Reading WAST/WASM from contracts/eosio.sudo/eosio.sudo.wasm...
Using already assembled WASM...
Publishing contract...
$ cat deploy_sudo_contract_trx.json
{
  "expiration": "2018-06-29T19:55:26",
  "ref_block_num": 18544,
  "ref_block_prefix": 562790588,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "setcode",
      "authorization": [{
          "actor": "eosio.sudo",
          "permission": "active"
        }
      ],
      "data": "00004d1a03ea30550000c8180061736d01000000013e0c60017f006000017e60027e7e0060017e006000017f60027f7f017f60027f7f0060037f7f7f017f60057f7e7f7f7f0060000060037e7e7e0060017f017f029d010803656e7610616374696f6e5f646174615f73697a65000403656e760c63757272656e745f74696d65000103656e760c656f73696f5f617373657274000603656e76066d656d637079000703656e7610726561645f616374696f6e5f64617461000503656e760c726571756972655f61757468000303656e760d726571756972655f6175746832000203656e760d73656e645f64656665727265640008030f0e0505050400000a05070b050b000904050170010202050301000107c7010b066d656d6f72790200165f5a6571524b3131636865636b73756d32353653315f0008165f5a6571524b3131636865636b73756d31363053315f0009165f5a6e65524b3131636865636b73756d31363053315f000a036e6f77000b305f5a4e35656f73696f3132726571756972655f6175746845524b4e535f31367065726d697373696f6e5f6c6576656c45000c155f5a4e35656f73696f347375646f34657865634576000d056170706c79000e066d656d636d700010066d616c6c6f630011046672656500140908010041000b02150d0a9a130e0b002000200141201010450b0b002000200141201010450b0d0020002001412010104100470b0a00100142c0843d80a70b0e002000290300200029030810060b9e0102017e027f410028020441206b2202210341002002360204200029030010050240024010002200418104490d002000101121020c010b410020022000410f6a4170716b22023602040b2002200010041a200041074b41101002200341186a2002410810031a2003290318100520032903182101200310013703002003200137030820032003290318200241086a2000410010074100200341206a3602040bfd0403027f047e017f4100410028020441206b220936020442002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b024020072002520d0042002106423b2105413021044200210703400240024002400240024020064204560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b200720015141c00010020b0240024020012000510d0042002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b20072002520d010b20092000370318200242808080808080a0aad700520d00200941003602142009410136021020092009290310370208200941186a200941086a100f1a0b4100200941206a3602040b8c0101047f4100280204220521042001280204210220012802002101024010002203450d00024020034180044d0d00200310112205200310041a200510140c010b410020052003410f6a4170716b22053602042005200310041a0b200020024101756a210302402002410171450d00200328020020016a28020021010b200320011100004100200436020441010b4901037f4100210502402002450d000240034020002d0000220320012d00002204470d01200141016a2101200041016a21002002417f6a22020d000c020b0b200320046b21050b20050b0900418001200010120bcd04010c7f02402001450d00024020002802c041220d0d004110210d200041c0c1006a41103602000b200141086a200141046a41077122026b200120021b210202400240024020002802c441220a200d4f0d002000200a410c6c6a4180c0006a21010240200a0d0020004184c0006a220d2802000d0020014180c000360200200d20003602000b200241046a210a034002402001280208220d200a6a20012802004b0d002001280204200d6a220d200d28020041808080807871200272360200200141086a22012001280200200a6a360200200d200d28020041808080807872360200200d41046a22010d030b2000101322010d000b0b41fcffffff0720026b2104200041c8c1006a210b200041c0c1006a210c20002802c8412203210d03402000200d410c6c6a22014188c0006a28020020014180c0006a22052802004641d0c200100220014184c0006a280200220641046a210d0340200620052802006a2107200d417c6a2208280200220941ffffffff07712101024020094100480d000240200120024f0d000340200d20016a220a20074f0d01200a280200220a4100480d012001200a41ffffffff07716a41046a22012002490d000b0b20082001200220012002491b200941808080807871723602000240200120024d0d00200d20026a200420016a41ffffffff07713602000b200120024f0d040b200d20016a41046a220d2007490d000b41002101200b4100200b28020041016a220d200d200c280200461b220d360200200d2003470d000b0b20010f0b2008200828020041808080807872360200200d0f0b41000b870501087f20002802c44121010240024041002d00a643450d0041002802a84321070c010b3f002107410041013a00a6434100200741107422073602a8430b200721030240024002400240200741ffff036a41107622023f0022084d0d00200220086b40001a4100210820023f00470d0141002802a84321030b41002108410020033602a84320074100480d0020002001410c6c6a210220074180800441808008200741ffff037122084181f8034922061b6a2008200741ffff077120061b6b20076b2107024041002d00a6430d003f002103410041013a00a6434100200341107422033602a8430b20024180c0006a210220074100480d01200321060240200741076a417871220520036a41ffff036a41107622083f0022044d0d00200820046b40001a20083f00470d0241002802a84321060b4100200620056a3602a8432003417f460d0120002001410c6c6a22014184c0006a2802002206200228020022086a2003460d020240200820014188c0006a22052802002201460d00200620016a2206200628020041808080807871417c20016b20086a72360200200520022802003602002006200628020041ffffffff07713602000b200041c4c1006a2202200228020041016a220236020020002002410c6c6a22004184c0006a200336020020004180c0006a220820073602000b20080f0b02402002280200220820002001410c6c6a22034188c0006a22012802002207460d0020034184c0006a28020020076a2203200328020041808080807871417c20076b20086a72360200200120022802003602002003200328020041ffffffff07713602000b2000200041c4c1006a220728020041016a22033602c0412007200336020041000f0b2002200820076a36020020020b7b01037f024002402000450d0041002802c04222024101480d004180c10021032002410c6c4180c1006a21010340200341046a2802002202450d010240200241046a20004b0d00200220032802006a20004b0d030b2003410c6a22032001490d000b0b0f0b2000417c6a2203200328020041ffffffff07713602000b0300000b0bcf01060041040b04b04900000041100b0572656164000041200b086f6e6572726f72000041300b06656f73696f000041c0000b406f6e6572726f7220616374696f6e277320617265206f6e6c792076616c69642066726f6d207468652022656f73696f222073797374656d206163636f756e74000041d0c2000b566d616c6c6f635f66726f6d5f6672656564207761732064657369676e656420746f206f6e6c792062652063616c6c6564206166746572205f686561702077617320636f6d706c6574656c7920616c6c6f636174656400"
    },{
      "account": "eosio",
      "name": "setabi",
      "authorization": [{
          "actor": "eosio.sudo",
          "permission": "active"
        }
      ],
      "data": "00004d1a03ea3055df040e656f73696f3a3a6162692f312e30030c6163636f756e745f6e616d65046e616d650f7065726d697373696f6e5f6e616d65046e616d650b616374696f6e5f6e616d65046e616d6506107065726d697373696f6e5f6c6576656c0002056163746f720c6163636f756e745f6e616d650a7065726d697373696f6e0f7065726d697373696f6e5f6e616d6506616374696f6e0004076163636f756e740c6163636f756e745f6e616d65046e616d650b616374696f6e5f6e616d650d617574686f72697a6174696f6e127065726d697373696f6e5f6c6576656c5b5d0464617461056279746573127472616e73616374696f6e5f68656164657200060a65787069726174696f6e0e74696d655f706f696e745f7365630d7265665f626c6f636b5f6e756d0675696e743136107265665f626c6f636b5f7072656669780675696e743332136d61785f6e65745f75736167655f776f7264730976617275696e743332106d61785f6370755f75736167655f6d730575696e74380964656c61795f7365630976617275696e74333209657874656e73696f6e000204747970650675696e74313604646174610562797465730b7472616e73616374696f6e127472616e73616374696f6e5f6865616465720314636f6e746578745f667265655f616374696f6e7308616374696f6e5b5d07616374696f6e7308616374696f6e5b5d167472616e73616374696f6e5f657874656e73696f6e730b657874656e73696f6e5b5d046578656300020865786563757465720c6163636f756e745f6e616d65037472780b7472616e73616374696f6e01000000000080545704657865630000000000"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

Once again, as described in sub-section 2.1.1, edit the values of the `ref_block_num` and `ref_block_prefix` fields to be 0 and edit the time of the `expiration` field to some point in the future that provides enough time to approve and execute the proposed transaction. After editing deploy_sudo_contract_trx.json the first few lines of it may look something like the following:
```
$ head -n 9 deploy_sudo_contract_trx.json
{
  "expiration": "2018-07-06T12:00:00",
  "ref_block_num": 0,
  "ref_block_prefix": 0,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
```

This guide will assume that there are 21 active block producers on the chain with account names: `blkproducera`, `blkproducerb`, ..., `blkproduceru`. The end of sub-section 2.1.1 displayed what the JSON of the active permissions of each of the active block producers would look like given the assumptions about the active block producer set. That JSON was stored in the file producer_permissions.json; if the approvers (i.e. block producers) have not created that file already, they should create that file now as shown at the end of sub-section 2.1.1.

#### 2.2.2 Propose the transaction to deploy the eosio.sudo contract

Only one of the potential approvers will need to propose the transaction that was created in the previous sub-section. All the other approvers should still follow the steps in the previous sub-section to generate the same deploy_sudo_contract_trx.json file as all the other approvers. They will need this to compare to the actual proposed transaction prior to approving.

The approvers are typically going to be the active block producers of the chain, so it makes sense that one of the block producers is elected as the leader to propose the actual transaction. Note that this lead block producer will need to incur the temporary RAM cost of proposing the transaction, but they will get the RAM back when the proposal has executed or has been canceled (which only the proposer can do prior to expiration).

This guide will assume that `blkproducera` was chosen as the lead block producer to propose the transaction.

The lead block producer (`blkproducera`) should propose the transaction stored in deploy_sudo_contract_trx.json:
```
$ cleos multisig propose_trx deploysudo producer_permissions.json deploy_sudo_contract_trx.json blkproducera
executed transaction: 9e50dd40eba25583a657ee8114986a921d413b917002c8fb2d02e2d670f720a8  4312 bytes  871 us
#    eosio.msig <= eosio.msig::propose          {"proposer":"blkproducera","proposal_name":"deploysudo","requested":[{"actor":"blkproducera","permis...
warning: transaction executed locally, but may not be confirmed by the network yet
```

#### 2.2.3 Review and approve the transaction to deploy the eosio.sudo contract

Each of the potential approvers of the proposed transaction (i.e. the active block producers) should first review the proposed transaction to make sure they are not approving anything that they do not agree to.

The proposed transaction can be reviewed using the `cleos multisig review` command:
```
$ cleos multisig review blkproducera deploysudo > deploy_sudo_contract_trx_to_review.json
$ cat deploy_sudo_contract_trx_to_review.json
{
  "proposal_name": "deploysudo",
  "packed_transaction": "c0593f5b00000000000000000000020000000000ea305500000040258ab2c20100004d1a03ea305500000000a8ed3232d41800004d1a03ea30550000c8180061736d01000000013e0c60017f006000017e60027e7e0060017e006000017f60027f7f017f60027f7f0060037f7f7f017f60057f7e7f7f7f0060000060037e7e7e0060017f017f029d010803656e7610616374696f6e5f646174615f73697a65000403656e760c63757272656e745f74696d65000103656e760c656f73696f5f617373657274000603656e76066d656d637079000703656e7610726561645f616374696f6e5f64617461000503656e760c726571756972655f61757468000303656e760d726571756972655f6175746832000203656e760d73656e645f64656665727265640008030f0e0505050400000a05070b050b000904050170010202050301000107c7010b066d656d6f72790200165f5a6571524b3131636865636b73756d32353653315f0008165f5a6571524b3131636865636b73756d31363053315f0009165f5a6e65524b3131636865636b73756d31363053315f000a036e6f77000b305f5a4e35656f73696f3132726571756972655f6175746845524b4e535f31367065726d697373696f6e5f6c6576656c45000c155f5a4e35656f73696f347375646f34657865634576000d056170706c79000e066d656d636d700010066d616c6c6f630011046672656500140908010041000b02150d0a9a130e0b002000200141201010450b0b002000200141201010450b0d0020002001412010104100470b0a00100142c0843d80a70b0e002000290300200029030810060b9e0102017e027f410028020441206b2202210341002002360204200029030010050240024010002200418104490d002000101121020c010b410020022000410f6a4170716b22023602040b2002200010041a200041074b41101002200341186a2002410810031a2003290318100520032903182101200310013703002003200137030820032003290318200241086a2000410010074100200341206a3602040bfd0403027f047e017f4100410028020441206b220936020442002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b024020072002520d0042002106423b2105413021044200210703400240024002400240024020064204560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b200720015141c00010020b0240024020012000510d0042002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b20072002520d010b20092000370318200242808080808080a0aad700520d00200941003602142009410136021020092009290310370208200941186a200941086a100f1a0b4100200941206a3602040b8c0101047f4100280204220521042001280204210220012802002101024010002203450d00024020034180044d0d00200310112205200310041a200510140c010b410020052003410f6a4170716b22053602042005200310041a0b200020024101756a210302402002410171450d00200328020020016a28020021010b200320011100004100200436020441010b4901037f4100210502402002450d000240034020002d0000220320012d00002204470d01200141016a2101200041016a21002002417f6a22020d000c020b0b200320046b21050b20050b0900418001200010120bcd04010c7f02402001450d00024020002802c041220d0d004110210d200041c0c1006a41103602000b200141086a200141046a41077122026b200120021b210202400240024020002802c441220a200d4f0d002000200a410c6c6a4180c0006a21010240200a0d0020004184c0006a220d2802000d0020014180c000360200200d20003602000b200241046a210a034002402001280208220d200a6a20012802004b0d002001280204200d6a220d200d28020041808080807871200272360200200141086a22012001280200200a6a360200200d200d28020041808080807872360200200d41046a22010d030b2000101322010d000b0b41fcffffff0720026b2104200041c8c1006a210b200041c0c1006a210c20002802c8412203210d03402000200d410c6c6a22014188c0006a28020020014180c0006a22052802004641d0c200100220014184c0006a280200220641046a210d0340200620052802006a2107200d417c6a2208280200220941ffffffff07712101024020094100480d000240200120024f0d000340200d20016a220a20074f0d01200a280200220a4100480d012001200a41ffffffff07716a41046a22012002490d000b0b20082001200220012002491b200941808080807871723602000240200120024d0d00200d20026a200420016a41ffffffff07713602000b200120024f0d040b200d20016a41046a220d2007490d000b41002101200b4100200b28020041016a220d200d200c280200461b220d360200200d2003470d000b0b20010f0b2008200828020041808080807872360200200d0f0b41000b870501087f20002802c44121010240024041002d00a643450d0041002802a84321070c010b3f002107410041013a00a6434100200741107422073602a8430b200721030240024002400240200741ffff036a41107622023f0022084d0d00200220086b40001a4100210820023f00470d0141002802a84321030b41002108410020033602a84320074100480d0020002001410c6c6a210220074180800441808008200741ffff037122084181f8034922061b6a2008200741ffff077120061b6b20076b2107024041002d00a6430d003f002103410041013a00a6434100200341107422033602a8430b20024180c0006a210220074100480d01200321060240200741076a417871220520036a41ffff036a41107622083f0022044d0d00200820046b40001a20083f00470d0241002802a84321060b4100200620056a3602a8432003417f460d0120002001410c6c6a22014184c0006a2802002206200228020022086a2003460d020240200820014188c0006a22052802002201460d00200620016a2206200628020041808080807871417c20016b20086a72360200200520022802003602002006200628020041ffffffff07713602000b200041c4c1006a2202200228020041016a220236020020002002410c6c6a22004184c0006a200336020020004180c0006a220820073602000b20080f0b02402002280200220820002001410c6c6a22034188c0006a22012802002207460d0020034184c0006a28020020076a2203200328020041808080807871417c20076b20086a72360200200120022802003602002003200328020041ffffffff07713602000b2000200041c4c1006a220728020041016a22033602c0412007200336020041000f0b2002200820076a36020020020b7b01037f024002402000450d0041002802c04222024101480d004180c10021032002410c6c4180c1006a21010340200341046a2802002202450d010240200241046a20004b0d00200220032802006a20004b0d030b2003410c6a22032001490d000b0b0f0b2000417c6a2203200328020041ffffffff07713602000b0300000b0bcf01060041040b04b04900000041100b0572656164000041200b086f6e6572726f72000041300b06656f73696f000041c0000b406f6e6572726f7220616374696f6e277320617265206f6e6c792076616c69642066726f6d207468652022656f73696f222073797374656d206163636f756e74000041d0c2000b566d616c6c6f635f66726f6d5f6672656564207761732064657369676e656420746f206f6e6c792062652063616c6c6564206166746572205f686561702077617320636f6d706c6574656c7920616c6c6f6361746564000000000000ea305500000000b863b2c20100004d1a03ea305500000000a8ed3232e90400004d1a03ea3055df040e656f73696f3a3a6162692f312e30030c6163636f756e745f6e616d65046e616d650f7065726d697373696f6e5f6e616d65046e616d650b616374696f6e5f6e616d65046e616d6506107065726d697373696f6e5f6c6576656c0002056163746f720c6163636f756e745f6e616d650a7065726d697373696f6e0f7065726d697373696f6e5f6e616d6506616374696f6e0004076163636f756e740c6163636f756e745f6e616d65046e616d650b616374696f6e5f6e616d650d617574686f72697a6174696f6e127065726d697373696f6e5f6c6576656c5b5d0464617461056279746573127472616e73616374696f6e5f68656164657200060a65787069726174696f6e0e74696d655f706f696e745f7365630d7265665f626c6f636b5f6e756d0675696e743136107265665f626c6f636b5f7072656669780675696e743332136d61785f6e65745f75736167655f776f7264730976617275696e743332106d61785f6370755f75736167655f6d730575696e74380964656c61795f7365630976617275696e74333209657874656e73696f6e000204747970650675696e74313604646174610562797465730b7472616e73616374696f6e127472616e73616374696f6e5f6865616465720314636f6e746578745f667265655f616374696f6e7308616374696f6e5b5d07616374696f6e7308616374696f6e5b5d167472616e73616374696f6e5f657874656e73696f6e730b657874656e73696f6e5b5d046578656300020865786563757465720c6163636f756e745f6e616d65037472780b7472616e73616374696f6e0100000000008054570465786563000000000000",
  "transaction": {
    "expiration": "2018-07-06T12:00:00",
    "ref_block_num": 0,
    "ref_block_prefix": 0,
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "actions": [{
        "account": "eosio",
        "name": "setcode",
        "authorization": [{
            "actor": "eosio.sudo",
            "permission": "active"
          }
        ],
        "data": {
          "account": "eosio.sudo",
          "vmtype": 0,
          "vmversion": 0,
          "code": "0061736d01000000013e0c60017f006000017e60027e7e0060017e006000017f60027f7f017f60027f7f0060037f7f7f017f60057f7e7f7f7f0060000060037e7e7e0060017f017f029d010803656e7610616374696f6e5f646174615f73697a65000403656e760c63757272656e745f74696d65000103656e760c656f73696f5f617373657274000603656e76066d656d637079000703656e7610726561645f616374696f6e5f64617461000503656e760c726571756972655f61757468000303656e760d726571756972655f6175746832000203656e760d73656e645f64656665727265640008030f0e0505050400000a05070b050b000904050170010202050301000107c7010b066d656d6f72790200165f5a6571524b3131636865636b73756d32353653315f0008165f5a6571524b3131636865636b73756d31363053315f0009165f5a6e65524b3131636865636b73756d31363053315f000a036e6f77000b305f5a4e35656f73696f3132726571756972655f6175746845524b4e535f31367065726d697373696f6e5f6c6576656c45000c155f5a4e35656f73696f347375646f34657865634576000d056170706c79000e066d656d636d700010066d616c6c6f630011046672656500140908010041000b02150d0a9a130e0b002000200141201010450b0b002000200141201010450b0d0020002001412010104100470b0a00100142c0843d80a70b0e002000290300200029030810060b9e0102017e027f410028020441206b2202210341002002360204200029030010050240024010002200418104490d002000101121020c010b410020022000410f6a4170716b22023602040b2002200010041a200041074b41101002200341186a2002410810031a2003290318100520032903182101200310013703002003200137030820032003290318200241086a2000410010074100200341206a3602040bfd0403027f047e017f4100410028020441206b220936020442002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b024020072002520d0042002106423b2105413021044200210703400240024002400240024020064204560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b200720015141c00010020b0240024020012000510d0042002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b20072002520d010b20092000370318200242808080808080a0aad700520d00200941003602142009410136021020092009290310370208200941186a200941086a100f1a0b4100200941206a3602040b8c0101047f4100280204220521042001280204210220012802002101024010002203450d00024020034180044d0d00200310112205200310041a200510140c010b410020052003410f6a4170716b22053602042005200310041a0b200020024101756a210302402002410171450d00200328020020016a28020021010b200320011100004100200436020441010b4901037f4100210502402002450d000240034020002d0000220320012d00002204470d01200141016a2101200041016a21002002417f6a22020d000c020b0b200320046b21050b20050b0900418001200010120bcd04010c7f02402001450d00024020002802c041220d0d004110210d200041c0c1006a41103602000b200141086a200141046a41077122026b200120021b210202400240024020002802c441220a200d4f0d002000200a410c6c6a4180c0006a21010240200a0d0020004184c0006a220d2802000d0020014180c000360200200d20003602000b200241046a210a034002402001280208220d200a6a20012802004b0d002001280204200d6a220d200d28020041808080807871200272360200200141086a22012001280200200a6a360200200d200d28020041808080807872360200200d41046a22010d030b2000101322010d000b0b41fcffffff0720026b2104200041c8c1006a210b200041c0c1006a210c20002802c8412203210d03402000200d410c6c6a22014188c0006a28020020014180c0006a22052802004641d0c200100220014184c0006a280200220641046a210d0340200620052802006a2107200d417c6a2208280200220941ffffffff07712101024020094100480d000240200120024f0d000340200d20016a220a20074f0d01200a280200220a4100480d012001200a41ffffffff07716a41046a22012002490d000b0b20082001200220012002491b200941808080807871723602000240200120024d0d00200d20026a200420016a41ffffffff07713602000b200120024f0d040b200d20016a41046a220d2007490d000b41002101200b4100200b28020041016a220d200d200c280200461b220d360200200d2003470d000b0b20010f0b2008200828020041808080807872360200200d0f0b41000b870501087f20002802c44121010240024041002d00a643450d0041002802a84321070c010b3f002107410041013a00a6434100200741107422073602a8430b200721030240024002400240200741ffff036a41107622023f0022084d0d00200220086b40001a4100210820023f00470d0141002802a84321030b41002108410020033602a84320074100480d0020002001410c6c6a210220074180800441808008200741ffff037122084181f8034922061b6a2008200741ffff077120061b6b20076b2107024041002d00a6430d003f002103410041013a00a6434100200341107422033602a8430b20024180c0006a210220074100480d01200321060240200741076a417871220520036a41ffff036a41107622083f0022044d0d00200820046b40001a20083f00470d0241002802a84321060b4100200620056a3602a8432003417f460d0120002001410c6c6a22014184c0006a2802002206200228020022086a2003460d020240200820014188c0006a22052802002201460d00200620016a2206200628020041808080807871417c20016b20086a72360200200520022802003602002006200628020041ffffffff07713602000b200041c4c1006a2202200228020041016a220236020020002002410c6c6a22004184c0006a200336020020004180c0006a220820073602000b20080f0b02402002280200220820002001410c6c6a22034188c0006a22012802002207460d0020034184c0006a28020020076a2203200328020041808080807871417c20076b20086a72360200200120022802003602002003200328020041ffffffff07713602000b2000200041c4c1006a220728020041016a22033602c0412007200336020041000f0b2002200820076a36020020020b7b01037f024002402000450d0041002802c04222024101480d004180c10021032002410c6c4180c1006a21010340200341046a2802002202450d010240200241046a20004b0d00200220032802006a20004b0d030b2003410c6a22032001490d000b0b0f0b2000417c6a2203200328020041ffffffff07713602000b0300000b0bcf01060041040b04b04900000041100b0572656164000041200b086f6e6572726f72000041300b06656f73696f000041c0000b406f6e6572726f7220616374696f6e277320617265206f6e6c792076616c69642066726f6d207468652022656f73696f222073797374656d206163636f756e74000041d0c2000b566d616c6c6f635f66726f6d5f6672656564207761732064657369676e656420746f206f6e6c792062652063616c6c6564206166746572205f686561702077617320636f6d706c6574656c7920616c6c6f636174656400"
        },
        "hex_data": "00004d1a03ea30550000c8180061736d01000000013e0c60017f006000017e60027e7e0060017e006000017f60027f7f017f60027f7f0060037f7f7f017f60057f7e7f7f7f0060000060037e7e7e0060017f017f029d010803656e7610616374696f6e5f646174615f73697a65000403656e760c63757272656e745f74696d65000103656e760c656f73696f5f617373657274000603656e76066d656d637079000703656e7610726561645f616374696f6e5f64617461000503656e760c726571756972655f61757468000303656e760d726571756972655f6175746832000203656e760d73656e645f64656665727265640008030f0e0505050400000a05070b050b000904050170010202050301000107c7010b066d656d6f72790200165f5a6571524b3131636865636b73756d32353653315f0008165f5a6571524b3131636865636b73756d31363053315f0009165f5a6e65524b3131636865636b73756d31363053315f000a036e6f77000b305f5a4e35656f73696f3132726571756972655f6175746845524b4e535f31367065726d697373696f6e5f6c6576656c45000c155f5a4e35656f73696f347375646f34657865634576000d056170706c79000e066d656d636d700010066d616c6c6f630011046672656500140908010041000b02150d0a9a130e0b002000200141201010450b0b002000200141201010450b0d0020002001412010104100470b0a00100142c0843d80a70b0e002000290300200029030810060b9e0102017e027f410028020441206b2202210341002002360204200029030010050240024010002200418104490d002000101121020c010b410020022000410f6a4170716b22023602040b2002200010041a200041074b41101002200341186a2002410810031a2003290318100520032903182101200310013703002003200137030820032003290318200241086a2000410010074100200341206a3602040bfd0403027f047e017f4100410028020441206b220936020442002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b024020072002520d0042002106423b2105413021044200210703400240024002400240024020064204560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b200720015141c00010020b0240024020012000510d0042002106423b2105412021044200210703400240024002400240024020064206560d0020042c00002203419f7f6a41ff017141194b0d01200341a5016a21030c020b420021082006420b580d020c030b200341d0016a41002003414f6a41ff01714105491b21030b2003ad42388642388721080b2008421f83200542ffffffff0f838621080b200441016a2104200642017c2106200820078421072005427b7c2205427a520d000b20072002520d010b20092000370318200242808080808080a0aad700520d00200941003602142009410136021020092009290310370208200941186a200941086a100f1a0b4100200941206a3602040b8c0101047f4100280204220521042001280204210220012802002101024010002203450d00024020034180044d0d00200310112205200310041a200510140c010b410020052003410f6a4170716b22053602042005200310041a0b200020024101756a210302402002410171450d00200328020020016a28020021010b200320011100004100200436020441010b4901037f4100210502402002450d000240034020002d0000220320012d00002204470d01200141016a2101200041016a21002002417f6a22020d000c020b0b200320046b21050b20050b0900418001200010120bcd04010c7f02402001450d00024020002802c041220d0d004110210d200041c0c1006a41103602000b200141086a200141046a41077122026b200120021b210202400240024020002802c441220a200d4f0d002000200a410c6c6a4180c0006a21010240200a0d0020004184c0006a220d2802000d0020014180c000360200200d20003602000b200241046a210a034002402001280208220d200a6a20012802004b0d002001280204200d6a220d200d28020041808080807871200272360200200141086a22012001280200200a6a360200200d200d28020041808080807872360200200d41046a22010d030b2000101322010d000b0b41fcffffff0720026b2104200041c8c1006a210b200041c0c1006a210c20002802c8412203210d03402000200d410c6c6a22014188c0006a28020020014180c0006a22052802004641d0c200100220014184c0006a280200220641046a210d0340200620052802006a2107200d417c6a2208280200220941ffffffff07712101024020094100480d000240200120024f0d000340200d20016a220a20074f0d01200a280200220a4100480d012001200a41ffffffff07716a41046a22012002490d000b0b20082001200220012002491b200941808080807871723602000240200120024d0d00200d20026a200420016a41ffffffff07713602000b200120024f0d040b200d20016a41046a220d2007490d000b41002101200b4100200b28020041016a220d200d200c280200461b220d360200200d2003470d000b0b20010f0b2008200828020041808080807872360200200d0f0b41000b870501087f20002802c44121010240024041002d00a643450d0041002802a84321070c010b3f002107410041013a00a6434100200741107422073602a8430b200721030240024002400240200741ffff036a41107622023f0022084d0d00200220086b40001a4100210820023f00470d0141002802a84321030b41002108410020033602a84320074100480d0020002001410c6c6a210220074180800441808008200741ffff037122084181f8034922061b6a2008200741ffff077120061b6b20076b2107024041002d00a6430d003f002103410041013a00a6434100200341107422033602a8430b20024180c0006a210220074100480d01200321060240200741076a417871220520036a41ffff036a41107622083f0022044d0d00200820046b40001a20083f00470d0241002802a84321060b4100200620056a3602a8432003417f460d0120002001410c6c6a22014184c0006a2802002206200228020022086a2003460d020240200820014188c0006a22052802002201460d00200620016a2206200628020041808080807871417c20016b20086a72360200200520022802003602002006200628020041ffffffff07713602000b200041c4c1006a2202200228020041016a220236020020002002410c6c6a22004184c0006a200336020020004180c0006a220820073602000b20080f0b02402002280200220820002001410c6c6a22034188c0006a22012802002207460d0020034184c0006a28020020076a2203200328020041808080807871417c20076b20086a72360200200120022802003602002003200328020041ffffffff07713602000b2000200041c4c1006a220728020041016a22033602c0412007200336020041000f0b2002200820076a36020020020b7b01037f024002402000450d0041002802c04222024101480d004180c10021032002410c6c4180c1006a21010340200341046a2802002202450d010240200241046a20004b0d00200220032802006a20004b0d030b2003410c6a22032001490d000b0b0f0b2000417c6a2203200328020041ffffffff07713602000b0300000b0bcf01060041040b04b04900000041100b0572656164000041200b086f6e6572726f72000041300b06656f73696f000041c0000b406f6e6572726f7220616374696f6e277320617265206f6e6c792076616c69642066726f6d207468652022656f73696f222073797374656d206163636f756e74000041d0c2000b566d616c6c6f635f66726f6d5f6672656564207761732064657369676e656420746f206f6e6c792062652063616c6c6564206166746572205f686561702077617320636f6d706c6574656c7920616c6c6f636174656400"
      },{
        "account": "eosio",
        "name": "setabi",
        "authorization": [{
            "actor": "eosio.sudo",
            "permission": "active"
          }
        ],
        "data": {
          "account": "eosio.sudo",
          "abi": "0e656f73696f3a3a6162692f312e30030c6163636f756e745f6e616d65046e616d650f7065726d697373696f6e5f6e616d65046e616d650b616374696f6e5f6e616d65046e616d6506107065726d697373696f6e5f6c6576656c0002056163746f720c6163636f756e745f6e616d650a7065726d697373696f6e0f7065726d697373696f6e5f6e616d6506616374696f6e0004076163636f756e740c6163636f756e745f6e616d65046e616d650b616374696f6e5f6e616d650d617574686f72697a6174696f6e127065726d697373696f6e5f6c6576656c5b5d0464617461056279746573127472616e73616374696f6e5f68656164657200060a65787069726174696f6e0e74696d655f706f696e745f7365630d7265665f626c6f636b5f6e756d0675696e743136107265665f626c6f636b5f7072656669780675696e743332136d61785f6e65745f75736167655f776f7264730976617275696e743332106d61785f6370755f75736167655f6d730575696e74380964656c61795f7365630976617275696e74333209657874656e73696f6e000204747970650675696e74313604646174610562797465730b7472616e73616374696f6e127472616e73616374696f6e5f6865616465720314636f6e746578745f667265655f616374696f6e7308616374696f6e5b5d07616374696f6e7308616374696f6e5b5d167472616e73616374696f6e5f657874656e73696f6e730b657874656e73696f6e5b5d046578656300020865786563757465720c6163636f756e745f6e616d65037472780b7472616e73616374696f6e01000000000080545704657865630000000000"
        },
        "hex_data": "00004d1a03ea3055df040e656f73696f3a3a6162692f312e30030c6163636f756e745f6e616d65046e616d650f7065726d697373696f6e5f6e616d65046e616d650b616374696f6e5f6e616d65046e616d6506107065726d697373696f6e5f6c6576656c0002056163746f720c6163636f756e745f6e616d650a7065726d697373696f6e0f7065726d697373696f6e5f6e616d6506616374696f6e0004076163636f756e740c6163636f756e745f6e616d65046e616d650b616374696f6e5f6e616d650d617574686f72697a6174696f6e127065726d697373696f6e5f6c6576656c5b5d0464617461056279746573127472616e73616374696f6e5f68656164657200060a65787069726174696f6e0e74696d655f706f696e745f7365630d7265665f626c6f636b5f6e756d0675696e743136107265665f626c6f636b5f7072656669780675696e743332136d61785f6e65745f75736167655f776f7264730976617275696e743332106d61785f6370755f75736167655f6d730575696e74380964656c61795f7365630976617275696e74333209657874656e73696f6e000204747970650675696e74313604646174610562797465730b7472616e73616374696f6e127472616e73616374696f6e5f6865616465720314636f6e746578745f667265655f616374696f6e7308616374696f6e5b5d07616374696f6e7308616374696f6e5b5d167472616e73616374696f6e5f657874656e73696f6e730b657874656e73696f6e5b5d046578656300020865786563757465720c6163636f756e745f6e616d65037472780b7472616e73616374696f6e01000000000080545704657865630000000000"
      }
    ],
    "transaction_extensions": []
  }
}
```

Each approver should be able to see that the proposed transaction is setting the code and ABI of the `eosio.sudo` contract. But the data is just hex data and therefore not very meaningful to the approver. And considering that `eosio.sudo` at this point should be a privileged contract, it would be very dangerous for block producers to just allow a contract to be deployed to a privileged account without knowing exactly which WebAssembly code they are deploying and also auditing the source code that generated that WebAssembly code to ensure it is safe to deploy.

This guide assumes that each approver has already audited the source code of the contract to be deployed and has already compiled that code to generate the WebAssembly code that should be byte-for-byte identical to the code that every other approver following the same process should have generated. The guide also assumes that this generated code and its associated ABI were provided in the steps in sub-section 2.2.1 that generated the transaction in the deploy_sudo_contract_trx.json file. It then becomes quite simple to verify that the proposed transaction is identical to the one the potential approver could have proposed with the code and ABI that they already audited:
```
$ cleos multisig propose_trx -j -s -d deploysudo '[]' deploy_sudo_contract_trx.json blkproducera | grep '"data":' | sed 's/^[ \t]*"data":[ \t]*//;s/[",]//g' | cut -c 35- > expected_deploy_sudo_trx_serialized.hex
$ cat expected_deploy_sudo_trx_serialized.hex | cut -c -50
c0593f5b00000000000000000000020000000000ea30550000
$ cat deploy_sudo_account_trx_to_review.json | grep '"packed_transaction":' | sed 's/^[ \t]*"packed_transaction":[ \t]*//;s/[",]//g' > proposed_deploy_sudo_trx_serialized.hex
$ cat proposed_deploy_sudo_trx_serialized.hex | cut -c -50
c0593f5b00000000000000000000020000000000ea30550000
$ diff expected_deploy_sudo_trx_serialized.hex proposed_deploy_sudo_trx_serialized.hex
```

When an approver (e.g. `blkproducerb`) is satisfied with the proposed transaction, they can simply approve it:
```
$ cleos multisig approve blkproducera deploysudo '{"actor": "blkproducerb", "permission": "active"}' -p blkproducerb
executed transaction: d1e424e05ee4d96eb079fcd5190dd0bf35eca8c27dd7231b59df8e464881abfd  128 bytes  483 us
#    eosio.msig <= eosio.msig::approve          {"proposer":"blkproducera","proposal_name":"deploysudo","level":{"actor":"blkproducerb","permission"...
warning: transaction executed locally, but may not be confirmed by the network yet
```

#### 2.2.4 Execute the transaction to create the eosio.sudo account

When the necessary approvals are collected (in this example, with 21 block producers, at least 15 of their approvals were required), anyone can push the `eosio.msig::exec` action which executes the approved transaction. It makes a lot of sense for the lead block producer who proposed the transaction to also execute it (this will incur another temporary RAM cost for the deferred transaction that is generated by the eosio.msig contract).

```
$ cleos multisig exec blkproducera deploysudo blkproducera
executed transaction: e8da14c6f1fdc3255b5413adccfd0d89b18f832a4cc18c4324ea2beec6abd483  160 bytes  1877 us
#    eosio.msig <= eosio.msig::exec             {"proposer":"blkproducera","proposal_name":"deploysudo","executer":"blkproducera"}
```

Anyone can now verify that the `eosio.sudo` contract was deployed correctly.

```
$ cleos get code -a retrieved-eosio.sudo.abi eosio.sudo
code hash: 1b3456a5eca28bcaca7a2a3360fbb2a72b9772a416c8e11a303bcb26bfe3263c
saving abi to retrieved-eosio.sudo.abi
$ sha256sum contracts/eosio.sudo/eosio.sudo.wasm
1b3456a5eca28bcaca7a2a3360fbb2a72b9772a416c8e11a303bcb26bfe3263c  contracts/eosio.sudo/eosio.sudo.wasm
```

If the two hashes match then the local WebAssembly code is the one deployed on the blockchain. The retrieved ABI, which was stored in the file retrieved-eosio.sudo.abi, can then be compared to the original ABI of the contract (contracts/eosio.sudo/eosio.sudo.abi) to ensure they are semantically the same.

## 3. Using the eosio.sudo contract

### 3.1 Example: Updating owner authority of an arbitrary account

This example will demonstrate how to use the deployed eosio.sudo contract together with the eosio.msig contract to allow a greater than two-thirds supermajority of block producers of an EOSIO blockchain to change the owner authority of an arbitrary account. The example will use cleos: in particular, the `cleos multisig` command, the `cleos set account permission` sub-command, and the `cleos sudo exec` sub-command. However, the guide also demonstrates what to do if the `cleos sudo exec` sub-command is not available.

This guide assumes that there are 21 active block producers on the chain with account names: `blkproducera`, `blkproducerb`, ..., `blkproduceru`. Block producer `blkproducera` will act as the lead block producer handling the proposal of the transaction.

The producer permissions will later come in handy when proposing a transaction that must be approved by a supermajority of the producers. So a file producer_permissions.json containing those permission (see contents below) should be created to be used later in this guide:
```
$ cat producer_permissions.json
[
   {"actor": "blkproducera", "permission": "active"},
   {"actor": "blkproducerb", "permission": "active"},
   {"actor": "blkproducerc", "permission": "active"},
   {"actor": "blkproducerd", "permission": "active"},
   {"actor": "blkproducere", "permission": "active"},
   {"actor": "blkproducerf", "permission": "active"},
   {"actor": "blkproducerg", "permission": "active"},
   {"actor": "blkproducerh", "permission": "active"},
   {"actor": "blkproduceri", "permission": "active"},
   {"actor": "blkproducerj", "permission": "active"},
   {"actor": "blkproducerk", "permission": "active"},
   {"actor": "blkproducerl", "permission": "active"},
   {"actor": "blkproducerm", "permission": "active"},
   {"actor": "blkproducern", "permission": "active"},
   {"actor": "blkproducero", "permission": "active"},
   {"actor": "blkproducerp", "permission": "active"},
   {"actor": "blkproducerq", "permission": "active"},
   {"actor": "blkproducerr", "permission": "active"},
   {"actor": "blkproducers", "permission": "active"},
   {"actor": "blkproducert", "permission": "active"},
   {"actor": "blkproduceru", "permission": "active"}
]
```

#### 3.1.1 Generate the transaction to change the owner permission of an account

The goal of this example is for the block producers to change the owner permission of the account `alice`.

The initial status of the `alice` account might be:
```
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:     49.74 KiB    used:     3.365 KiB  

net bandwidth:
     staked:          1.0000 SYS           (total stake delegated from account to self)
     delegated:       0.0000 SYS           (total staked delegated to account from others)
     used:                 0 bytes
     available:        2.304 MiB  
     limit:            2.304 MiB  

cpu bandwidth:
     staked:          1.0000 SYS           (total stake delegated from account to self)
     delegated:       0.0000 SYS           (total staked delegated to account from others)
     used:                 0 us   
     available:        460.8 ms   
     limit:            460.8 ms   

producers:     <not voted>
```

Assume that none of the block producers know the private key corresponding to the public key `EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV` which, as can be seen above, is initially securing access to the `alice` account.

The first step is to generate the transaction changing the owner permission of the `alice` account as if `alice` is authorizing the change:
```
$ cleos set account permission -s -j -d alice owner '{"threshold": 1, "accounts": [{"permission": {"actor": "eosio", "permission": "active"}, "weight": 1}]}' > update_alice_owner_trx.json
```

Then modify update_alice_owner_trx.json so that the values for the `ref_block_num` and `ref_block_prefix` fields are both 0 and the value of the `expiration` field is `"1970-01-01T00:00:00"`:
```
$ cat update_alice_owner_trx.json
{
  "expiration": "1970-01-01T00:00:00",
  "ref_block_num": 0,
  "ref_block_prefix": 0,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "updateauth",
      "authorization": [{
          "actor": "alice",
          "permission": "active"
        }
      ],
      "data": "0000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed3232010000"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

The next step is to generate the transaction containing the `eosio.sudo::exec` action. This action will contain the transaction in update_alice_owner_trx.json as part of its action payload data.

```
$ cleos sudo exec -s -j -d blkproducera update_alice_owner_trx.json > sudo_update_alice_owner_trx.json
```

Once again modify sudo_update_alice_owner_trx.json so that the value for the `ref_block_num` and `ref_block_prefix` fields are both 0. However, instead of changing the value of the expiration field to `"1970-01-01T00:00:00"`, it should be changed to a time that is far enough in the future to allow enough time for the proposed transaction to be approved and executed.
```
$ cat sudo_update_alice_owner_trx.json
{
  "expiration": "2018-07-06T12:00:00",
  "ref_block_num": 0,
  "ref_block_prefix": 0,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio.sudo",
      "name": "exec",
      "authorization": [{
          "actor": "blkproducera",
          "permission": "active"
        },{
          "actor": "eosio.sudo",
          "permission": "active"
        }
      ],
      "data": "60ae423ad15b613c0000000000000000000000000000010000000000ea30550040cbdaa86c52d5010000000000855c3400000000a8ed3232310000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed323201000000"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

If the `cleos sudo` command is not available, there is an alternative way to generate the above transaction. There is no need to continue reading the remaining of sub-section 3.1.1 if the sudo_update_alice_owner_trx.json file was already generated with content similar to the above using the `cleos sudo exec` sub-command method.

First the hex encoding of the binary serialization of the transaction in update_alice_owner_trx.json must be obtained. One way of obtaining this data is through the following command:
```
$ cleos multisig propose_trx -s -j -d nothing '[]' update_alice_owner_trx.json nothing | grep '"data":' | sed 's/^[ \t]*"data":[ \t]*//;s/[",]//g' | cut -c 35- > update_alice_owner_trx_serialized.hex
$ cat update_alice_owner_trx_serialized.hex
0000000000000000000000000000010000000000ea30550040cbdaa86c52d5010000000000855c3400000000a8ed3232310000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed323201000000
```

Then generate the template for the transaction containing the `eosio.sudo::exec` action:
```
$ cleos push action -s -j -d eosio.sudo exec '{"executer": "blkproducera", "trx": ""}' > sudo_update_alice_owner_trx.json
$ cat sudo_update_alice_owner_trx.json
{
  "expiration": "2018-06-29T23:34:01",
  "ref_block_num": 23708,
  "ref_block_prefix": 3605208482,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio.sudo",
      "name": "exec",
      "authorization": [],
      "data": "60ae423ad15b613c"
    }
  ],
  "transaction_extensions": [],
  "signatures": [],
  "context_free_data": []
}
```

Then modify the transaction in sudo_update_alice_owner_trx.json as follows:
  * replace the values of the `ref_block_num` and `ref_block_prefix` fields to 0;
  * replace the time of the `expiration` field to the desired expiration time as described above (e.g. `"2018-07-06T12:00:00"`);
  * append the hex data from update_alice_owner_trx_serialized.hex to the end of the existing hex data in the `data` field in sudo_update_alice_owner_trx.json.


#### 3.1.2 Propose the transaction to change the owner permission of an account

The lead block producer (`blkproducera`) should propose the transaction stored in sudo_update_alice_owner_trx.json:
```
$ cleos multisig propose_trx updatealice producer_permissions.json sudo_update_alice_owner_trx.json blkproducera
executed transaction: 10474f52c9e3fc8e729469a577cd2fc9e4330e25b3fd402fc738ddde26605c13  624 bytes  782 us
#    eosio.msig <= eosio.msig::propose          {"proposer":"blkproducera","proposal_name":"updatealice","requested":[{"actor":"blkproducera","permi...
warning: transaction executed locally, but may not be confirmed by the network yet
```

#### 3.1.3 Review and approve the transaction to change the owner permission of an account

Each of the potential approvers of the proposed transaction (i.e. the active block producers) should first review the proposed transaction to make sure they are not approving anything that they do not agree to.
```
$ cleos multisig review blkproducera updatealice > sudo_update_alice_owner_trx_to_review.json
$ cat sudo_update_alice_owner_trx_to_review.json
{
  "proposal_name": "updatealice",
  "packed_transaction": "c0593f5b000000000000000000000100004d1a03ea305500000000008054570260ae423ad15b613c00000000a8ed323200004d1a03ea305500000000a8ed32326b60ae423ad15b613c0000000000000000000000000000010000000000ea30550040cbdaa86c52d5010000000000855c3400000000a8ed3232310000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed32320100000000",
  "transaction": {
    "expiration": "2018-07-06T12:00:00",
    "ref_block_num": 0,
    "ref_block_prefix": 0,
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "actions": [{
        "account": "eosio.sudo",
        "name": "exec",
        "authorization": [{
            "actor": "blkproducera",
            "permission": "active"
          },{
            "actor": "eosio.sudo",
            "permission": "active"
          }
        ],
        "data": {
          "executer": "blkproducera",
          "trx": {
            "expiration": "1970-01-01T00:00:00",
            "ref_block_num": 0,
            "ref_block_prefix": 0,
            "max_net_usage_words": 0,
            "max_cpu_usage_ms": 0,
            "delay_sec": 0,
            "context_free_actions": [],
            "actions": [{
                "account": "eosio",
                "name": "updateauth",
                "authorization": [{
                    "actor": "alice",
                    "permission": "active"
                  }
                ],
                "data": "0000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed3232010000"
              }
            ],
            "transaction_extensions": []
          }
        },
        "hex_data": "60ae423ad15b613c0000000000000000000000000000010000000000ea30550040cbdaa86c52d5010000000000855c3400000000a8ed3232310000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed323201000000"
      }
    ],
    "transaction_extensions": []
  }
}
```

The approvers should go through the human-readable transaction output and make sure everything looks fine. However, due to a current limitation of nodeos/cleos, the JSONification of action payload data does not occur recursively. So while both the `hex_data` and the human-readable JSON `data` of the payload of the `eosio.sudo::exec` action is available in the output of the `cleos multisig review` command, only the hex data is available of the payload of the inner `eosio::updateauth` action. So it is not clear what the `updateauth` will actually do.

Furthermore, even if this usability issue was fixed in nodeos/cleos, there will still be cases where there is no sensible human-readable version of an action data payload within a transaction. An example of this is the proposed transaction in sub-section 2.2.3 which used the `eosio::setcode` action to set the contract code of the `eosio.sudo` account. The best thing to do in such situations is for the reviewer to compare the proposed transaction to one generated by them through a process they trust.

Since each block producer generated a transaction in sub-section 3.1.1 (stored in the file sudo_update_alice_owner_trx.json) which should be identical to the transaction proposed by the lead block producer, they can each simply check to see if the two transactions are identical:
```
$ cleos multisig propose_trx -j -s -d updatealice '[]' sudo_update_alice_owner_trx.json blkproducera | grep '"data":' | sed 's/^[ \t]*"data":[ \t]*//;s/[",]//g' | cut -c 35- > expected_sudo_update_alice_owner_trx_serialized.hex
$ cat expected_sudo_update_alice_owner_trx_serialized.hex
c0593f5b000000000000000000000100004d1a03ea305500000000008054570260ae423ad15b613c00000000a8ed323200004d1a03ea305500000000a8ed32326b60ae423ad15b613c0000000000000000000000000000010000000000ea30550040cbdaa86c52d5010000000000855c3400000000a8ed3232310000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed32320100000000
$ cat sudo_update_alice_owner_trx_to_review.json | grep '"packed_transaction":' | sed 's/^[ \t]*"packed_transaction":[ \t]*//;s/[",]//g' > proposed_sudo_update_alice_owner_trx_serialized.hex
$ cat proposed_sudo_update_alice_owner_trx_serialized.hex
c0593f5b000000000000000000000100004d1a03ea305500000000008054570260ae423ad15b613c00000000a8ed323200004d1a03ea305500000000a8ed32326b60ae423ad15b613c0000000000000000000000000000010000000000ea30550040cbdaa86c52d5010000000000855c3400000000a8ed3232310000000000855c340000000080ab26a700000000000000000100000000010000000000ea305500000000a8ed32320100000000
$ diff expected_sudo_update_alice_owner_trx_serialized.hex  proposed_sudo_update_alice_owner_trx_serialized.hex
```

When an approver (e.g. `blkproducerb`) is satisfied with the proposed transaction, they can simply approve it:
```
$ cleos multisig approve blkproducera updatealice '{"actor": "blkproducerb", "permission": "active"}' -p blkproducerb
executed transaction: 2bddc7747e0660ba26babf95035225895b134bfb2ede32ba0a2bb6091c7dab56  128 bytes  543 us
#    eosio.msig <= eosio.msig::approve          {"proposer":"blkproducera","proposal_name":"updatealice","level":{"actor":"blkproducerb","permission...
warning: transaction executed locally, but may not be confirmed by the network yet
```

#### 3.1.4 Execute the transaction to change the owner permission of an account

When the necessary approvals are collected (in this example, with 21 block producers, at least 15 of their approvals were required), anyone can push the `eosio.msig::exec` action which executes the approved transaction. It makes a lot of sense for the lead block producer who proposed the transaction to also execute it (this will incur another temporary RAM cost for the deferred transaction that is generated by the eosio.msig contract).

```
$ cleos multisig exec blkproducera updatealice blkproducera
executed transaction: 7127a66ae307fbef6415bf60c3e91a88b79bcb46030da983c683deb2a1a8e0d0  160 bytes  820 us
#    eosio.msig <= eosio.msig::exec             {"proposer":"blkproducera","proposal_name":"updatealice","executer":"blkproducera"}
warning: transaction executed locally, but may not be confirmed by the network yet
```

Anyone can now verify that the owner authority of `alice` was successfully changed:
```
$ cleos get account alice
permissions:
     owner     1:    1 eosio@active,
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:     49.74 KiB    used:     3.348 KiB  

net bandwidth:
     staked:          1.0000 SYS           (total stake delegated from account to self)
     delegated:       0.0000 SYS           (total staked delegated to account from others)
     used:                 0 bytes
     available:        2.304 MiB  
     limit:            2.304 MiB  

cpu bandwidth:
     staked:          1.0000 SYS           (total stake delegated from account to self)
     delegated:       0.0000 SYS           (total staked delegated to account from others)
     used:               413 us   
     available:        460.4 ms   
     limit:            460.8 ms   

producers:     <not voted>

```
