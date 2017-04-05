
Introduction
------------

The `debug_node` is a tool to allow developers to run many interesting sorts of "what-if" tests using state from a production blockchain.
Like "what happens if I produce enough blocks for the next hardfork time to arrive?" or "what would happen if this account (which I don't have a private key for) did this transaction?"

Setup
-----

Be sure you've built the right build targets:

    $ make get_dev_key debug_node cli_wallet witness_node

Use the `get_dev_key` utility to generate a keypair:

    $ programs/genesis_util/get_dev_key "" nathan
    [{"private_key":"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3","public_key":"BTS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","address":"BTSFAbAx7yuxt725qSZvfwWqkdCwp9ZnUama"}]

Obtain a copy of the blockchain in `block_db` directory:
    $ programs/witness_node/witness_node --data-dir data/mydatadir
    # ... wait for chain to sync
    ^C
    $ cp -Rp data/mydatadir/blockchain/database/block_num_to_block ./block_db

Set up a new datadir with the following `config.ini` settings:

    # setup API endpoint
    rpc-endpoint = 127.0.0.1:8090
    # setting this to empty effectively disables the p2p network
    seed-nodes = []
    # set apiaccess.json so we can set up
    api-access = "data/debug_datadir/api-access.json"

Then set up `data/debug_datadir/api-access.json` to allow access to the debug API like this:

    {
       "permission_map" :
       [
          [
             "bytemaster",
             {
                "password_hash_b64" : "9e9GF7ooXVb9k4BoSfNIPTelXeGOZ5DrgOYMj94elaY=",
                "password_salt_b64" : "INDdM6iCi/8=",
                "allowed_apis" : ["database_api", "network_broadcast_api", "history_api", "network_node_api", "debug_api"]
             }
          ],
          [
             "*",
             {
                "password_hash_b64" : "*",
                "password_salt_b64" : "*",
                "allowed_apis" : ["database_api", "network_broadcast_api", "history_api"]
             }
          ]
       ]
    }

See [here](https://github.com/cryptonomex/graphene#accessing-restricted-apis) for more detail on the `api-access.json` format.

Once that is set up, run `debug_node` against your newly prepared datadir:

    programs/debug_node/debug_node --data-dir data/debug_datadir

Run `cli_wallet` to connect to the `debug_node` port, using the username and password to access the new `debug_api` (and also a different wallet file):

    programs/cli_wallet/cli_wallet -s 127.0.0.1:8090 -w debug.wallet -u bytemaster -p supersecret

Example usage
-------------

Load some blocks from the datadir:

    dbg_push_blocks block_db 20000

Note, when pushing a very large number of blocks sometimes `cli_wallet` hangs and you must Ctrl+C and restart it (leaving the `debug_node` running).

Generate (fake) blocks with our own private key:

    dbg_generate_blocks 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 1000

Update `angel` account to be controlled by our own private key and generate a (fake) transfer:

    dbg_update_object {"_action":"update", "id":"1.2.1090", "active":{"weight_threshold":1,"key_auths":[["BTS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",1]]}}
    import_key angel 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
    transfer angel init0 999999 BTS "" true

How it works
------------

The commands work by creating diff(s) from the main chain that are applied to the local chain at specified block height(s).  It lets you easily check out "what-if"
scenarios in a fantasy debug toy world forked from the real chain, e.g. "if we take all of the blocks until today, then generate a bunch more until a hardfork time
in the future arrives, does the chain stay up?  Can I do transactions X, Y, and Z in the wallet after the hardfork?"  Anyone connecting to this node sees the same
fantasy world, so you can e.g. make changes with the `cli_wallet` and see them exist in other `cli_wallet` instances (or GUI wallets or API scripts).

Limitations
-----------

The main limitations are:

- No export format for the diffs, so you can't really [1] connect multiple `debug_node` to each other.
- Once faked block(s) or tx(s) have been produced on your chain, you can't really [1] stream blocks or tx's from the main network to your chain.

[1] It should theoretically be possible, but it's non-trivial and totally untested.
