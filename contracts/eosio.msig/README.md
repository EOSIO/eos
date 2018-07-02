eosio.msig
--------

Actions:
The naming convention is codeaccount::actionname followed by a list of paramters.

Create a proposal
## eosio.msig::propose    proposer proposal_name requested trx
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal (should be unique for proposer)
   - **requested** permission levels expected to approve the proposal
   - **trx** proposed transaction

   Storage changes are billed to 'proposer'

Approve a proposal
## eosio.msig::approve    proposer proposal_name level
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **level** permission level approving the transaction

   Storage changes are billed to 'proposer'

Revoke an approval of transaction
## eosio.msig::unapprove    proposer proposal_name level
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **level** permission level revoking approval from the transaction

   Storage changes are billed to 'proposer'

Cancel a proposal
## eosio.msig::cancel    proposer proposal_name canceler
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **canceler** account canceling the transaction (only proposer can cancel not expired transaction)

Execute a proposal
## eosio.msig::exec    proposer proposal_name executer
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **executer** account executing the transaction


Cleos usage example.

Prerequisites:
   - eosio.token contract installed to eosio.token account, eosio.msig contract installed on eosio.msig account which is a privileged account.
   - account 'treasury' is the issuer of EOS token.
   - account 'tester' exists.
   - keys to accounts 'treasury' and 'tester' imported into local wallet, the wallet is unlocked.

One user creates a proposal:
````
$ cleos multisig propose test '[{"actor": "treasury", "permission": "active"}]' '[{"actor": "treasury", "permission": "active"}]' eosio.token issue '{"to": "tester", "quantity": "1000.0000 EOS", "memo": ""}' -p tester
executed transaction: e26f3a3a7cba524a7b15a0b6c77c7daa73d3ba9bf84e83f9c2cdf27fcb183d61  336 bytes  107520 cycles
#    eosio.msig <= eosio.msig::propose          {"proposer":"tester","proposal_name":"test","requested":[{"actor":"treasury","permission":"active"}]...
````

Another user reviews the transaction:
````
$ cleos multisig review tester test -p treasury
{
  "proposal_name": "test",
  "requested_approvals": [{
      "actor": "treasury",
      "permission": "active"
    }
  ],
  "provided_approvals": [],
  "packed_transaction": "00aee75a0000000000000000000000000100a6823403ea30550000000000a5317601000000fe6a6cd4cd00000000a8ed323219000000005c95b1ca809698000000000004454f530000000000",
  "transaction": {
    "expiration": "2018-05-01T00:00:00",
    "region": 0,
    "ref_block_num": 0,
    "ref_block_prefix": 0,
    "max_net_usage_words": 0,
    "max_kcpu_usage": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "actions": [{
        "account": "eosio.token",
        "name": "issue",
        "authorization": [{
            "actor": "treasury",
            "permission": "active"
          }
        ],
        "data": {
          "to": "tester",
          "quantity": "1000.0000 EOS",
          "memo": ""
        },
        "hex_data": "000000005c95b1ca809698000000000004454f530000000000"
      }
    ]
  }
}
````

And then approves it:
````
$ cleos multisig approve tester test '{"actor": "treasury", "permission": "active"}' -p treasury
executed transaction: 475970a4b0016368d0503d1ce01577376f91f5a5ba63dd4353683bd95101b88d  256 bytes  108544 cycles
#    eosio.msig <= eosio.msig::approve          {"proposer":"tester","proposal_name":"test","level":{"actor":"treasury","permission":"active"}}
````

First user initiates execution:
````
$ cleos multisig exec tester test -p tester
executed transaction: 64e5eaceb77362694055f572ae35876111e87b637a55250de315b1b55e56d6c2  248 bytes  109568 cycles
#    eosio.msig <= eosio.msig::exec             {"proposer":"tester","proposal_name":"test","executer":"tester"}
````
