# Nodeos Plugins

## Overview

Plugins extend the core functionality implemented in `nodeos`. Some plugins are mandatory, such as `chain_plugin`, `net_plugin`, and `producer_plugin`, which reflect the modular design of `nodeos`. The other plugins are optional as they provide nice to have features, but non-essential for the nodes operation.

For information on specific plugins, just select from the list below:

* [`chain_api_plugin`](chain_api_plugin/index.md)
* [`chain_plugin`](chain_plugin/index.md)
* [`db_size_api_plugin`](db_size_api_plugin/index.md)
* [`faucet_testnet_plugin`](faucet_testnet_plugin/index.md)
* [`history_api_plugin`](history_api_plugin/index.md)
* [`history_plugin`](history_plugin/index.md)
* [`http_client_plugin`](http_client_plugin/index.md)
* [`http_plugin`](http_plugin/index.md)
* [`login_plugin`](login_plugin/index.md)
* [`net_api_plugin`](net_api_plugin/index.md)
* [`net_plugin`](net_plugin/index.md)
* [`producer_plugin`](producer_plugin/index.md)
* [`state_history_plugin`](state_history_plugin/index.md)
* [`test_control_api_plugin`](test_control_api_plugin/index.md)
* [`test_control_plugin`](test_control_plugin/index.md)
* [`txn_test_gen_plugin`](txn_test_gen_plugin/index.md)

[[info | Nodeos is modular]]
| Plugins add incremental functionality to `nodeos`. Unlike runtime plugins, `nodeos` plugins are built at compile-time.
