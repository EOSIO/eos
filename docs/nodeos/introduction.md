EOSIO comes with a number of programs.  The primary ones that you will use, and the ones that are covered here, are:

* `nodeos` (node + eos = nodeos)  - the core EOSIO **node** daemon that can be configured with plugins to run a node. Example uses are block production, dedicated API endpoints, and local development. 
* `cleos` (cli + eos = cleos) - command line interface to interact with the blockchain and to manage wallets.
* `keosd` (key + eos = keosd) - component that securely stores EOSIO keys in wallets. 

The basic relationship between these components is illustrated in the following diagram.

![EOSIO components](eosio_components.png)

In the sections that follow, you will build the EOSIO components, and deploy them in a single host, single node test network (testnet) configuration.
