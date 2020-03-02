---
content_title: EOSIO Deprecations
---

## Table tracking significant deprecations in EOSIO software

| Feature                                               | Date of Deprecation Notice |                    Deprecation Announcement Link                   | Date of Removal |                      Removal Announcement Link                     | Alternative to Consider                     |
|-------------------------------------------------------|:--------------------------:|:------------------------------------------------------------------:|:---------------:|:------------------------------------------------------------------:|---------------------------------------------|
| history_plugin                                        |         2018/08/14         |     [v1.2.0](https://github.com/EOSIO/eos/releases/tag/v1.2.0)     |        -        |                                  -                                 | state_history_plugin                        |
| eosiocpp                                              |         2018/08/14         |     [v1.2.0](https://github.com/EOSIO/eos/releases/tag/v1.2.0)     |    2019/03/13   |     [v1.7.0](https://github.com/EOSIO/eos/releases/tag/v1.7.0)     | eosio.cdt                                   |
| RAM billing within notified contracts                 |         2018/08/28         |     [v1.2.3](https://github.com/EOSIO/eos/releases/tag/v1.2.3)     |    See notes    |                              See notes                             | See notes                                   |
| binaryen                                              |         2018/09/18         |     [v1.3.0](https://github.com/EOSIO/eos/releases/tag/v1.3.0)     |    2018/10/16   |     [v1.4.0](https://github.com/EOSIO/eos/releases/tag/v1.4.0)     | wabt                                        |
| Docker files                                          |         2018/11/14         |                                  -                                 |    2019/06/28   |     [v1.8.0](https://github.com/EOSIO/eos/releases/tag/v1.8.0)     | binary releases                             |
| Send-to-self authorization bypass                     |         2018/12/13         |     [v1.4.5](https://github.com/EOSIO/eos/releases/tag/v1.4.5)     |    See notes    |                              See notes                             | See notes                                   |
| Fedora 27, Mint 18, Amazon Linux 1, macOS High Sierra |         2019/04/03         |     [v1.7.1](https://github.com/EOSIO/eos/releases/tag/v1.7.1)     |    2019/06/28   |     [v1.8.0](https://github.com/EOSIO/eos/releases/tag/v1.8.0)     | Ubuntu 18.04 (or other supported platforms) |
| bnet_plugin                                           |         2019/04/03         |     [v1.7.1](https://github.com/EOSIO/eos/releases/tag/v1.7.1)     |    2019/06/28   |     [v1.8.0](https://github.com/EOSIO/eos/releases/tag/v1.8.0)     | net_plugin                                  |
| mongo_db_plugin                                       |         2019/04/29         | [v1.8.0-rc1](https://github.com/EOSIO/eos/releases/tag/v1.8.0-rc1) |        -        |                                  -                                 | state_history_plugin                        |
| network-version-match option in net_plugin            |         2019/09/10         |     [v1.8.3](https://github.com/EOSIO/eos/releases/tag/v1.8.3)     |    2019/10/07   | [v2.0.0-rc1](https://github.com/EOSIO/eos/releases/tag/v2.0.0-rc1) | No alternative needed                       |
| Deferred transactions                                 |         2019/10/07         | [v2.0.0-rc1](https://github.com/EOSIO/eos/releases/tag/v2.0.0-rc1) |        -        |                                  -                                 | See notes                                   |
| WAVM runtime                                          |         2019/10/07         | [v2.0.0-rc1](https://github.com/EOSIO/eos/releases/tag/v2.0.0-rc1) |    2020/01/10   |     [v2.0.0](https://github.com/EOSIO/eos/releases/tag/v2.0.0)     | See notes                                   |


## Additional notes on some deprecated features

### history_plugin and mongo_db_plugin

The `history_plugin` was deprecated on August 12, 2018. The mongo_db_plugin was deprecated on April 29, 2019. Both of these plugins are still available in EOSIO software, but they are not recommended for use. Instead one can use a history solution built on top of the `state_history_plugin` which became stable on June 28, 2019.

### eosiocpp

The original WASM compiler for EOSIO smart contracts was called `eosiocpp` (not to be confused with the newer `eosio-cpp` compiler). The compiler along with the libraries to help with smart contract development were initially kept in the EOSIO repository. The compiler and the smart contract libraries were improved upon and moved into a separate repository containing the tools needed for EOSIO smart contract development: [eosio.cdt](https://github.com/EOSIO/eosio.cdt). The old WASM compiler and smart contract libraries were removed from the EOSIO repository in [v1.7.0](https://github.com/EOSIO/eos/releases/tag/v1.7.0). Contract developers are now expected to use the [eosio.cdt](https://github.com/EOSIO/eosio.cdt) to build smart contracts.

### RAM billing within notified contracts

The base EOSIO protocol allows contracts executing within a notification context to bill RAM to users included in the authorizations attached to the action they are notified of. This feature ultimately proved to be undesirable because of the burden it placed on users and contract developers to protect the RAM of their accounts from being consumed by malicious contracts. In [v1.2.3](https://github.com/EOSIO/eos/releases/tag/v1.2.3) of the reference EOSIO software, an optional subjective mitigation (enabled by default) was added to restrict contracts from billing RAM to other accounts when executing within a notification context. Furthermore, this behavior was deprecated with the plan to eventually enforce the restriction objectively in a future protocol upgrade. In [v1.8.0](https://github.com/EOSIO/eos/releases/tag/v1.8.0), support for the [`RAM_RESTRICTIONS`](https://github.com/EOSIO/eos/pull/7131) protocol feature was added which, when activated, makes the restriction an official part of the protocol but also enables flexibilities in how RAM usage can change during contract execution.

### Send-to-self authorization bypass

The base EOSIO protocol affords contracts an authorization bypass for actions that sent to itself (either via inline actions or deferred transactions). However, this authorization bypass had a flaw that enabled a privilege escalation enabling a contract to bill RAM usage to arbitrary EOSIO accounts. Therefore, in the [v1.4.5](https://github.com/EOSIO/eos/releases/tag/v1.4.5) and [v1.5.1](https://github.com/EOSIO/eos/releases/tag/v1.5.1) security releases of EOSIO (released on December 13, 2018), the authorization bypass behavior was severely restricted through a subjective mitigation. There was one exception allowed to minimize the impact of the restriction on existing contracts: an inline action sent by a contract to itself could carry forward an authorization included in the parent action.

However, all authorization bypasses (including the one exception made in the subjective mitigation) were announced as deprecated along with the releases on December 13, 2018 with the plan to enforce the restrictions objectively in a future protocol upgrade. In [v1.8.0](https://github.com/EOSIO/eos/releases/tag/v1.8.0), support for the [`RESTRICT_ACTION_TO_SELF`](https://github.com/EOSIO/eos/pull/7088) protocol feature was added which, when activated, changes the EOSIO protocol to remove all authorization bypasses that exist in the base protocol.

### Deferred transactions

Contract authors are encouraged to not use deferred transactions. Those interested in migrating to newer design paradigms can consider the options presented in the [EOSIO Specifications Repository](https://github.com/EOSIO/spec-repo/blob/master/esr_deprecate_deferred.md).

### WAVM runtime

The WAVM WebAssembly runtime is not recommended for use on live nodes on public EOSIO networks. You can consider using WAVM to speed up replays until EOS VM becomes stable, at which point the tier-up solution of EOS VM Optimized Compiler should be considered as a replacement for WAVM.
