---
content_title: EOSIO v2.1.0-rc3 Release Notes
---

This is a ***RELEASE CANDIDATE*** for version 2.1.0.

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

### Consolidated Security Fixes for v2.1.0-rc3 ([#9869](https://github.com/EOSIO/eos/pull/9869))
- Fixes to packed_transaction cache
- Transaction account fail limit refactor

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

## Stability bug fixes
- ([#9864](https://github.com/EOSIO/eos/pull/9864)) fix incorrect transaction_extensions declaration
- ([#9880](https://github.com/EOSIO/eos/pull/9880)) Fix ship big vector serialization
- ([#9896](https://github.com/EOSIO/eos/pull/9896)) Fix state_history zlib_unpack bug
- ([#9909](https://github.com/EOSIO/eos/pull/9909)) Fix state_history::length_writer
- ([#9986](https://github.com/EOSIO/eos/pull/9986)) EPE-389 fix net_plugin stall during head_catchup - merge release/2.1.x
- ([#9988](https://github.com/EOSIO/eos/pull/9988)) refactor kv get rows 2.1.x
- ([#9989](https://github.com/EOSIO/eos/pull/9989)) Explicit ABI conversion of signed_transaction - merge 2.1.x
- ([#10027](https://github.com/EOSIO/eos/pull/10027)) EPE-165: Improve logic for unlinkable blocks while sync'ing
- ([#10028](https://github.com/EOSIO/eos/pull/10028)) use p2p address for duplicate connection resolution

## Other changes
- ([#9858](https://github.com/EOSIO/eos/pull/9858)) Fix problem when using ubuntu libpqxx package
- ([#9863](https://github.com/EOSIO/eos/pull/9863)) chain_plugin db intrinsic table RPC calls incorrectly handling --lower and --upper in certain scenarios
- ([#9882](https://github.com/EOSIO/eos/pull/9882)) merge back fix build problem on cmake3.10
- ([#9884](https://github.com/EOSIO/eos/pull/9884)) Fix problem with libpqxx 7.3.0 upgrade
- ([#9893](https://github.com/EOSIO/eos/pull/9893)) EOS VM OC: Support LLVM 11 - 2.1
- ([#9900](https://github.com/EOSIO/eos/pull/9900)) Create Docker image with the eos binary and push to Dockerhub
- ([#9906](https://github.com/EOSIO/eos/pull/9906)) Add log path for unsupported log version exception
- ([#9930](https://github.com/EOSIO/eos/pull/9930)) Fix intermittent forked chain test failure
- ([#9931](https://github.com/EOSIO/eos/pull/9931)) trace history log messages should print nicely in syslog
- ([#9942](https://github.com/EOSIO/eos/pull/9942)) Fix "cleos net peers" command error
- ([#9943](https://github.com/EOSIO/eos/pull/9943)) Create eosio-debug-build Pipeline
- ([#9953](https://github.com/EOSIO/eos/pull/9953)) EPE-482 Fixed warning due to unreferenced label
- ([#9956](https://github.com/EOSIO/eos/pull/9956)) PowerTools is now powertools in CentOS 8.3 - 2.1
- ([#9958](https://github.com/EOSIO/eos/pull/9958)) merge back PR 9898 fix non-root build script for ensure-libpq...
- ([#9959](https://github.com/EOSIO/eos/pull/9959)) merge back PR 9899, try using oob cmake so as to save building time
- ([#9970](https://github.com/EOSIO/eos/pull/9970)) Updating to the new Docker hub repo EOSIO instead EOS
- ([#9975](https://github.com/EOSIO/eos/pull/9975)) Release/2.1.x: Add additional contract to test_exhaustive_snapshot
- ([#9983](https://github.com/EOSIO/eos/pull/9983)) Add warning interval option for resource monitor plugin
- ([#9994](https://github.com/EOSIO/eos/pull/9994)) Add unit tests for new fields added for get account in PR#9838
- ([#10014](https://github.com/EOSIO/eos/pull/10014)) [release 2.1.x] Fix LRT triggers
- ([#10020](https://github.com/EOSIO/eos/pull/10020)) revert changes to empty string as present for lower_bound, upper_bound,or index_value
- ([#10031](https://github.com/EOSIO/eos/pull/10031)) [release 2.1.x] Fix MacOS base image failures
- ([#10042](https://github.com/EOSIO/eos/pull/10042)) [release 2.1.x] Updated Mojave libpqxx dependency
- ([#10046](https://github.com/EOSIO/eos/pull/10046)) Reduce Docker Hub Manifest Queries
- ([#10054](https://github.com/EOSIO/eos/pull/10054)) Fix multiversion test failure - merge 2.1.x

## Documentation
- ([#9825](https://github.com/EOSIO/eos/pull/9825)) [docs] add how to: local testnet with consensus
- ([#9908](https://github.com/EOSIO/eos/pull/9908)) Add MacOS 10.15 (Catalina) to list of supported OSs in README
- ([#9914](https://github.com/EOSIO/eos/pull/9914)) [docs] add improvements based on code review 
- ([#9921](https://github.com/EOSIO/eos/pull/9921)) [docs] 2.1.x local testnet with consensus
- ([#9925](https://github.com/EOSIO/eos/pull/9925)) [docs] cleos doc-a-thon feedback
- ([#9933](https://github.com/EOSIO/eos/pull/9933)) [docs] cleos doc-a-thon feedback 2
- ([#9934](https://github.com/EOSIO/eos/pull/9934)) [docs] cleos doc-a-thon feedback 3
- ([#9938](https://github.com/EOSIO/eos/pull/9938)) [docs] cleos doc-a-thon feedback 4
- ([#9952](https://github.com/EOSIO/eos/pull/9952)) [docs] 2.1.x - improve annotation for db_update_i64
- ([#10009](https://github.com/EOSIO/eos/pull/10009)) [docs] Update various cleos how-tos and fix index - 2.1
