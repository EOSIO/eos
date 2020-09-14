---
content_title: EOSIO v2.0.7 Release Notes
---

This release contains stability and miscellaneous fixes.

## Stability bug fixes
- ([#9223](https://github.com/EOSIO/eos/pull/9223)) Fix log of pending block producer - 2.0

## Changes

### The following logger has been added: `transaction_failure_tracing`. ([#9252](https://github.com/EOSIO/eos/pull/9252))

A detailed log that emits failed verdicts from relay nodes on the P2P network. In addition, it adds a logging statement for a failed signature condition.

### New config file option: `max-nonprivileged-inline-action-size`. ([#9262](https://github.com/EOSIO/eos/pull/9262))

This option is the cap for the size of an inline action above which a transaction will subjectively fail; the default value is 4KB.

## Other Changes
- ([#9170](https://github.com/EOSIO/eos/pull/9170)) Fix onblock trace tracking - 2.0
- ([#9201](https://github.com/EOSIO/eos/pull/9201)) [2.0.x] Anka version bump
- ([#9265](https://github.com/EOSIO/eos/pull/9265)) Remove Concurrency Groups for Scheduled Builds

## Documentation
- ([#9124](https://github.com/EOSIO/eos/pull/9124)) Update the authority example
- ([#9275](https://github.com/EOSIO/eos/pull/9275)) [docs] New threshold for non privileged inline actions - 2.0
- ([#9288](https://github.com/EOSIO/eos/pull/9288)) [docs] Fix character formatting in nodeos CLI option - 2.0
- ([#9290](https://github.com/EOSIO/eos/pull/9290)) [docs] Correct Producer API title in RPC reference - 2.0


***
# EOSIO v2.0.6 Release Notes

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

- ([#9172](https://github.com/EOSIO/eos/pull/9172)) Escape Unicode C1 control code points.

Note: These security fixes are relevant to API nodes on EOSIO blockchain networks.

## Stability bug fixes

- ([#9065](https://github.com/EOSIO/eos/pull/9065)) Fix for cleos and keosd race condition - 2.0
- ([#9089](https://github.com/EOSIO/eos/pull/9089)) make ship WA key serialization match expected serialization - 2.0
- ([#9095](https://github.com/EOSIO/eos/pull/9095)) fix gcc10 build due to libyubihsm problem - 2.0
- ([#9127](https://github.com/EOSIO/eos/pull/9127)) Fix onblock handling in trace_api_plugin - 2.0
- ([#9129](https://github.com/EOSIO/eos/pull/9129)) GCC 8.3 on CentOS 7 compiler workaround - 2.0
- ([#9128](https://github.com/EOSIO/eos/pull/9128)) Restore abi_serializer backward compatibility - 2.0

## Changes

### Add more information in trace-api-plugin responses for better usage. ([#9005](https://github.com/EOSIO/eos/pull/9005))

Adds `transaction_mroot`, `action_mroot` and `schedule_version` in block trace. Also adds `status`, `cpu_usage_us`, `net_usage_words`, `signatures`, and `transaction_header` in transaction trace.

### New RPC endpoint **`get_accounts_by_authorizers`** ([#8899](https://github.com/EOSIO/eos/pull/8899))

New optional RPC endpoint **`POST /v1/chain/get_accounts_by_authorizers`** added to `chain_api_plugin` that provides a super-set of the deprecated `history_api_plugin`'s `get_key_accounts` and `get_controlled_accounts` RPC methods.

Flag to enable endpoint (default false): **`--enable-account-queries`**

## Other Changes

- ([#8975](https://github.com/EOSIO/eos/pull/8975)) failing nodeos_run_test when core symbol is not SYS - 2.0
- ([#9002](https://github.com/EOSIO/eos/pull/9002)) Support Triggering a Build that Runs ALL Tests in One Build
- ([#9007](https://github.com/EOSIO/eos/pull/9007)) Improved reporting in nodeos_forked_chain_lr_test - 2.0.x
- ([#9013](https://github.com/EOSIO/eos/pull/9013)) Bugfix for uninitialized variable in cleos - 2.0
- ([#9009](https://github.com/EOSIO/eos/pull/9009)) Upgrade CLI11 to 1.9.0 - 2.0
- ([#9028](https://github.com/EOSIO/eos/pull/9028)) Fix keosd auto-launching after CLI11 upgrade - 2.0
- ([#9035](https://github.com/EOSIO/eos/pull/9035)) For Release 2.0 - Updated the priority of the APIs in producer_api_plugin and net_api_plugin to MEDIUM_HIGH
- ([#9049](https://github.com/EOSIO/eos/pull/9049)) add rapidjson license to install - 2.0
- ([#9052](https://github.com/EOSIO/eos/pull/9052)) Print stderr if keosd_auto_launch_test.py fails - 2.0
- ([#9060](https://github.com/EOSIO/eos/pull/9060)) Fix uninitialized struct members used as CLI flags - 2.0
- ([#9062](https://github.com/EOSIO/eos/pull/9062)) Fix timedelta and strftime usage - 2.0
- ([#9078](https://github.com/EOSIO/eos/pull/9078)) Update date in LICENSE - 2.0
- ([#9063](https://github.com/EOSIO/eos/pull/9063)) add help text to wasm-runtime - 2.0.x
- ([#9084](https://github.com/EOSIO/eos/pull/9084)) Add support for specifing a logging.json to keosd - 2.0
- ([#9082](https://github.com/EOSIO/eos/pull/9082)) Add change type to pull request template - 2.0
- ([#8899](https://github.com/EOSIO/eos/pull/8899)) Account Query DB : Proposal to maintain get_(key|controlled)_accounts [2.0]
- ([#9103](https://github.com/EOSIO/eos/pull/9103)) Add default contract name clarifier in how to deploy smart contract - 2.0
- ([#9109](https://github.com/EOSIO/eos/pull/9109)) [2.0.x] Bump Anka plugin version and timeouts.
- ([#9115](https://github.com/EOSIO/eos/pull/9115)) Simplify create_snapshot POST request - 2.0
- ([#9110](https://github.com/EOSIO/eos/pull/9110)) Update algorithm for determining number of parallel jobs - 2.0

## Documentation

- ([#8980](https://github.com/EOSIO/eos/pull/8980)) Add nodeos RPC API index, improve nodeos implementation doc, fix link - 2.0
- ([#8995](https://github.com/EOSIO/eos/pull/8995)) Update example logging.json - 2.0
- ([#9102](https://github.com/EOSIO/eos/pull/9102)) Fix inaccurate nodeos reference in wallet_api_plugin - 2.0
- ([#9116](https://github.com/EOSIO/eos/pull/9116)) Replace inaccurate wording in how to replay from snapshot - 2.0
- ([#9113](https://github.com/EOSIO/eos/pull/9113)) Add trace_api logger to docs - 2.0
- ([#9142](https://github.com/EOSIO/eos/pull/9142)) Add missing reference to RPC API index [docs] - 2.0
- ([#9141](https://github.com/EOSIO/eos/pull/9141)) Fix Trace API reference request/response inaccuracies [docs] - 2.0
- ([#9144](https://github.com/EOSIO/eos/pull/9144)) Fix title case issue in keosd how-to [docs] - 2.0
- ([#9145](https://github.com/EOSIO/eos/pull/9145)) Add conditional step in state history plugin how-to [docs] - 2.0

## Thanks!

Special thanks to the community contributors that submitted patches for this release:
- @cc32d9
- @oldcold


***
# EOSIO v2.0.5 Release Notes

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

### Consolidated Security Fixes for 2.0.5 ([#8983](https://github.com/EOSIO/eos/pull/8983))

- EOS-VM security fixes

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

## Stability bug fixes

- ([#8826](https://github.com/EOSIO/eos/pull/8826)) trace_api_plugin yield timeout - 2.0
- ([#8836](https://github.com/EOSIO/eos/pull/8836)) fix potential leak in OC's wrapped_fd move assignment op - 2.0

## Changes

### Trace API Compressed Data Log Support ([#8826](https://github.com/EOSIO/eos/pull/8826), [#8837](https://github.com/EOSIO/eos/pull/8837), [#8881](https://github.com/EOSIO/eos/pull/8881))

Compressed file support was added to `trace_api_plugin`. See ([#8837](https://github.com/EOSIO/eos/pull/8837)) for more details.

The RPC call `v1/trace_api/get_block` now has "async" http support. Therefore, executing `get_block` no longer runs on the main application thread but on the configurable `http-threads` thread pool.

Additionally, `trace_api_plugin` now respects `http-max-response-time-ms` for limiting response time of RPC call `v1/trace_api/get_block`. It is very likely that the default value of `http-max-response-time-ms` will not be appropriate for large blocks and will need to be increased.

## Other Changes

- ([#8822](https://github.com/EOSIO/eos/pull/8822)) Merge minimize logging changes to 2.0.x 
- ([#8823](https://github.com/EOSIO/eos/pull/8823)) yield_function for abi_serializer - 2.0
- ([#8855](https://github.com/EOSIO/eos/pull/8855)) Improve too many bytes in flight error info - 2.0
- ([#8861](https://github.com/EOSIO/eos/pull/8861)) HTTP Plugin async APIs [2.0]
- ([#8873](https://github.com/EOSIO/eos/pull/8873)) Fix spurious HTTP related test failure [2.0] (round 3)
- ([#8883](https://github.com/EOSIO/eos/pull/8883)) wabt: don't search for python because we don't run tests - 2.0
- ([#8884](https://github.com/EOSIO/eos/pull/8884)) Correctly Sanitize git Branch and Tag Names
- ([#8894](https://github.com/EOSIO/eos/pull/8894)) Increase get info priority to medium high
- ([#8889](https://github.com/EOSIO/eos/pull/8889)) Sync from snapshot - 2.0
- ([#8906](https://github.com/EOSIO/eos/pull/8906)) Remove assert check for error code 400 - release 2.0.x
- ([#8944](https://github.com/EOSIO/eos/pull/8944)) noop change to macos-10.14-unpinned.sh to regen CI image, 2.0
- ([#8941](https://github.com/EOSIO/eos/pull/8941)) replace boost::bind with std::bind, fixing boost 1.73beta builds - 2.0
- ([#8954](https://github.com/EOSIO/eos/pull/8954)) llvm 10 support for EOS VM OC - 2.0
- ([#8949](https://github.com/EOSIO/eos/pull/8949)) Replace bc with shell arithmetic - 2.0
- ([#8962](https://github.com/EOSIO/eos/pull/8962)) tests/get_table_tests.cpp: incorrect use of CORE_SYM_STR - 2.0
- ([#8963](https://github.com/EOSIO/eos/pull/8963)) Make /bin/df ignore $BLOCKSIZE - 2.0
- ([#8952](https://github.com/EOSIO/eos/pull/8952)) Fix SHIP block delay - 2.0
- ([#8972](https://github.com/EOSIO/eos/pull/8972)) Add possibility to run .cicd scripts from different environments (2.0.x Backport)
- ([#8968](https://github.com/EOSIO/eos/pull/8968)) Support Running ALL Tests in One Build

## Documentation changes

- ([#8825](https://github.com/EOSIO/eos/pull/8825)) remove leading $ chars from shell codeblocks in README.md - 2.0
- ([#8835](https://github.com/EOSIO/eos/pull/8835)) Trace API documentation update - 2.0
- ([#8843](https://github.com/EOSIO/eos/pull/8843)) Fix double titles for release 2.0.x
- ([#8845](https://github.com/EOSIO/eos/pull/8845)) [docs] trace api reference api correction - 2.0
- ([#8918](https://github.com/EOSIO/eos/pull/8918)) Updates to manual build instructions - 2.0

## Thanks!

Special thanks to the community contributors that submitted patches for this release:
- @cc32d9
- @maoueh


***
# EOSIO v2.0.4 Release Notes

This release contains stability and miscellaneous fixes.

## Deprecation Notices

The `read-only` option for the `read-mode` parameter in `nodeos` has been deprecated. It is possible to achieve the same behavior with `read-mode = head`, `p2p-accept-transactions = false`, and `api-accept-transactions = false`. See the sub-section "Accept transactions options" below for more details.

Please refer to the [Consolidated EOSIO Deprecations List](https://github.com/EOSIO/eos/issues/7597) for the currently active set of deprecation notices. 

## Stability bug fixes

- ([#8684](https://github.com/EOSIO/eos/pull/8684)) Net plugin sync priority - 2.0
- ([#8729](https://github.com/EOSIO/eos/pull/8729)) Get info priority - 2.0

## Changes

### Trace API Plugin ([#8800](https://github.com/EOSIO/eos/pull/8800))

This release contains the first official release of the Trace API Plugin.  This plugin is an optional addition to `nodeos` that stores a tailored view of the transactions and actions that execute on the chain retrievable at a block level.  The Trace API focuses on operational maintainability storing data on the filesystem instead of in RAM like the deprecated `history-plugin` and organizing that data such that operators can easily prune old data without disrupting operations.  

For more information, see the PR notes and the official documentation.

### Exit transaction early when there is insufficient account CPU ([#8638](https://github.com/EOSIO/eos/pull/8638))

`nodeos` no longer considers a transaction for inclusion in a block in the process of being produced if the billed account(s) do not have sufficient CPU available to cover the previously estimated CPU usage of the transaction (only if a previous estimate for CPU usage is available).

### Produce block immediately if resource limits are exhausted ([#8651](https://github.com/EOSIO/eos/pull/8651), [#8673](https://github.com/EOSIO/eos/pull/8673))

`nodeos` now immediately produces a block if either the CPU or NET usage thresholds are exceeded. This change includes a fix for dropping late blocks starting 50ms earlier than the block production window.

New options:
* `max-block-cpu-usage-threshold-us`:
Threshold (in microseconds) of CPU block production to consider block full; when accumulated CPU usage within a block is less than `max-block-cpu-usage-threshold-us` away from `max-block-cpu-usage`, the block can be produced immediately. Default value is 5000.
* `max-block-net-usage-threshold-bytes`:
Threshold (in bytes) of NET block production to consider block full; when accumulated NET usage within a block is less than `max-block-net-usage-threshold-us` away from `max-block-net-usage`, the block can be produced immediately. Default value is 1024.

### Accept transactions options ([#8702](https://github.com/EOSIO/eos/pull/8702))

New options:
* `p2p-accept-transactions`: Allow transactions received over p2p
network to be evaluated and relayed if valid. Default is true.
* `api-accept-transactions`: Allow API transactions to be evaluated
and relayed if valid. Default is true.

Provides ability to have a `read-mode = head` with `p2p-accept-transactions = false` and `api-accept-transactions = true`. This combination creates an efficient API node that is not burdened with processing P2P transactions.

The same behavior of the now deprecated `read-mode = read-only` can be achieved with `read-mode = head` by setting `p2p-accept-transactions = false` and `api-accept-transactions = false`.

**WARNING:** Use of `read-mode = irreversible` now requires setting `p2p-accept-transactions = false` and `api-accept-transactions = false` to avoid assertion at startup.

### Relay block early ([#8701](https://github.com/EOSIO/eos/pull/8701))

Improve block relaying performance when a block is from a trusted producer or if `nodeos` is running in light validation mode. This is achieved by relaying the block as soon as block header validation is complete (but before full block application/validation).

## Other Changes

- ([#8654](https://github.com/EOSIO/eos/pull/8654)) Fix format message. - 2.0
- ([#8668](https://github.com/EOSIO/eos/pull/8668)) Add troubleshooting item for PREACTIVATE_FEATURE protocol
- ([#8689](https://github.com/EOSIO/eos/pull/8689)) incoming-defer-ratio description - 2.0
- ([#8695](https://github.com/EOSIO/eos/pull/8695)) [2.0.x] Community PR tweaks.
- ([#8700](https://github.com/EOSIO/eos/pull/8700)) [2.0.x] Base images pipeline.
- ([#8714](https://github.com/EOSIO/eos/pull/8714)) [2.0.x] Actions rerun fixes.
- ([#8710](https://github.com/EOSIO/eos/pull/8710)) Add block producing explainer doc - 2.0
- ([#8721](https://github.com/EOSIO/eos/pull/8721)) Fix multiple version protocol test intermittent failure - 2.0
- ([#8727](https://github.com/EOSIO/eos/pull/8727)) Update the getting started link [merge 2]
- ([#8752](https://github.com/EOSIO/eos/pull/8752)) chain_api_plugin swagger file - 2.0
- ([#8756](https://github.com/EOSIO/eos/pull/8756)) Fixes #8600 clean up nodeos options section
- ([#8757](https://github.com/EOSIO/eos/pull/8757)) link cleos net status reference doc with the peer network protocol doc
- ([#8590](https://github.com/EOSIO/eos/pull/8590)) db_size_api_plugin swagger file - 2.0
- ([#8591](https://github.com/EOSIO/eos/pull/8591)) net_api_plugin swagger file - 2.0
- ([#8592](https://github.com/EOSIO/eos/pull/8592)) producer_api_plugin swagger file - 2.0
- ([#8593](https://github.com/EOSIO/eos/pull/8593)) test_control_api_plugin swagger file - 2.0
- ([#8754](https://github.com/EOSIO/eos/pull/8754)) swagger configuration for docs - 2.0
- ([#8762](https://github.com/EOSIO/eos/pull/8762)) Fix broken link in producer plugin docs - 2.0
- ([#8763](https://github.com/EOSIO/eos/pull/8763)) Fix wasm-runtime option parameters - 2.0
- ([#8765](https://github.com/EOSIO/eos/pull/8765)) Add Incoming-defer-ratio description - 2.0
- ([#8767](https://github.com/EOSIO/eos/pull/8767)) Fix other blocks.log callout - 2.0
- ([#8768](https://github.com/EOSIO/eos/pull/8768)) Improve create account description - 2.0
- ([#8782](https://github.com/EOSIO/eos/pull/8782)) link to librt when using posix timers - 2.0
- ([#8781](https://github.com/EOSIO/eos/pull/8781)) free unknown EOS VM OC codegen versions from the code cache - 2.0
- ([#8794](https://github.com/EOSIO/eos/pull/8794)) disable EOS VM on non-x86 platforms - 2.0
- ([#8803](https://github.com/EOSIO/eos/pull/8803)) Expire blacklisted scheduled transactions by LIB time - 2.0
- ([#8811](https://github.com/EOSIO/eos/pull/8811)) Add initial Trace API plugin docs to nodeos - 2.0
- ([#8814](https://github.com/EOSIO/eos/pull/8814)) Add RPC Trace API plugin reference to nodeos - 2.0


***
# EOSIO v2.0.3 Release Notes

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

### Consolidated Security Fixes for 2.0.3 ([#8643](https://github.com/EOSIO/eos/pull/8643))

- Add deadline to base58 encoding.

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

## Stability bug fixes

- ([#8617](https://github.com/EOSIO/eos/pull/8617)) Init net_plugin member variables - 2.0

## Other Changes

- ([#8606](https://github.com/EOSIO/eos/pull/8606)) Skip sync from genesis and resume from state test on tagged builds
- ([#8612](https://github.com/EOSIO/eos/pull/8612)) [2.0.x] Actions for community PRs.
- ([#8633](https://github.com/EOSIO/eos/pull/8633)) remove brew's python@2 install - 2.0
- ([#8636](https://github.com/EOSIO/eos/pull/8636)) bump script's macos version check to 10.14 - 2.0

## Deprecation notice reminder

Please refer to the [Consolidated EOSIO Deprecations List](https://github.com/EOSIO/eos/issues/7597) for the currently active set of deprecation notices. 


***
# EOSIO v2.0.2 Release Notes

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

### Consolidated Security Fixes for 2.0.2 ([#8595](https://github.com/EOSIO/eos/pull/8595))

- Restrict allowed block signature types.

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

## Stability bug fixes

- ([#8526](https://github.com/EOSIO/eos/pull/8526)) Handle socket close before async callback - 2.0
- ([#8546](https://github.com/EOSIO/eos/pull/8546)) Net plugin dispatch - 2.0
- ([#8552](https://github.com/EOSIO/eos/pull/8552)) Net plugin unlinkable blocks - 2.0
- ([#8560](https://github.com/EOSIO/eos/pull/8560)) Backport of 8056 to 2.0
- ([#8561](https://github.com/EOSIO/eos/pull/8561)) Net plugin post - 2.0
- ([#8564](https://github.com/EOSIO/eos/pull/8564)) Delayed production time - 2.0

## Changes

### Limit block production window ([#8571](https://github.com/EOSIO/eos/pull/8571), [#8578](https://github.com/EOSIO/eos/pull/8578))

The new options `cpu-effort-percent` and `last-block-cpu-effort-percent` now provide a mechanism to restrict the amount of time a producer is processing transactions for inclusion into a block. It also controls the time a producer will finalize/produce and transmit a block. Block construction now always begins at whole or half seconds for the next block.

### Stricter signature parsing 

Versions of EOSIO prior to v2.0.x accept keys and signatures containing extra data at the end. In EOSIO v2.0.x, the Base58 string parser performs additional sanity checks on keys and signatures to make sure they do not contain more data than necessary. These stricter checks cause nodes to reject signatures generated by some transaction signing libraries; eosjs v20 is known to generate proper signatures. For more information see issue [#8534](https://github.com/EOSIO/eos/issues/8534).

## Other Changes

- ([#8555](https://github.com/EOSIO/eos/pull/8555)) Drop late check - 2.0
- ([#8557](https://github.com/EOSIO/eos/pull/8557)) Read-only with drop-late-block - 2.0
- ([#8568](https://github.com/EOSIO/eos/pull/8568)) Timestamp watermark slot - 2.0.x
- ([#8577](https://github.com/EOSIO/eos/pull/8577)) [release 2.0.x] Documentation patch 1 update
- ([#8583](https://github.com/EOSIO/eos/pull/8583)) P2p read only - 2.0
- ([#8589](https://github.com/EOSIO/eos/pull/8589)) Producer plugin log - 2.0

## Deprecation notice reminder

Please refer to the [Consolidated EOSIO Deprecations List](https://github.com/EOSIO/eos/issues/7597) for the currently active set of deprecation notices. 


***
# EOSIO v2.0.1 Release Notes

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

### Consolidated Security Fixes for 2.0.1 ([#8514](https://github.com/EOSIO/eos/pull/8514))

- Earlier block validation for greater security.
- Improved handling of deferred transactions during block production.
- Reduce net plugin logging and handshake size limits.

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

## Stability bug fixes

- ([#8471](https://github.com/EOSIO/eos/pull/8471)) Remove new block id notify feature - 2.0
- ([#8472](https://github.com/EOSIO/eos/pull/8472)) Report block header diff when digests do not match - 2.0
- ([#8496](https://github.com/EOSIO/eos/pull/8496)) Drop late blocks - 2.0
- ([#8510](https://github.com/EOSIO/eos/pull/8510)) http_plugin shutdown - 2.0

## Other Changes

- ([#8430](https://github.com/EOSIO/eos/pull/8430)) Update fc to fix crash in logging
- ([#8435](https://github.com/EOSIO/eos/pull/8435)) [release/2.0.x] Update README.md and hotfix documentation links
- ([#8452](https://github.com/EOSIO/eos/pull/8452)) [2.0.x] [CI/CD] Boost will not install without SDKROOT
- ([#8457](https://github.com/EOSIO/eos/pull/8457)) [2.0.x] reverting fc
- ([#8458](https://github.com/EOSIO/eos/pull/8458)) [2.0.x] Pipeline file for testing the build script
- ([#8467](https://github.com/EOSIO/eos/pull/8467)) [2.0.x] Switching to using the EOSIO fork of anka-buildkite-plugin for security reasons
- ([#8515](https://github.com/EOSIO/eos/pull/8515)) [2.0.x] Don't trigger LRT a second time

## Deprecation notice reminder

Please refer to the [Consolidated EOSIO Deprecations List](https://github.com/EOSIO/eos/issues/7597) for the currently active set of deprecation notices. 


***
# EOSIO v2.0.0 Release Notes

This release contains security, stability, and miscellaneous fixes.

This release also includes an EOSIO consensus upgrade. Please see the "Consensus Protocol Upgrades" section below for details. These protocol upgrades necessitate a change to the state database structure which requires a nodeos upgrade process that involves more steps than in the case of most minor release upgrades. For details on the upgrade process, see the "Upgrading from previous versions of EOSIO" section below.

## Security bug fixes

### Consolidated Security Fixes for 2.0.0 ([#8420](https://github.com/EOSIO/eos/pull/8420))

- Limit size of response to API requests
- EOS VM fixes

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

The above is in addition to the other security fixes that were introduced in prior release candidates for 2.0.0:

-  ([#8195](https://github.com/EOSIO/eos/pull/8195)) Consolidated Security Fixes for 2.0.0-rc2 
-  ([#8344](https://github.com/EOSIO/eos/pull/8344)) Consolidated Security Fixes for 2.0.0-rc3 

## Stability bug fixes

- ([#8399](https://github.com/EOSIO/eos/pull/8399)) Net plugin sync check - 2.0

This above is in addition to the other stability fixes that were introduced in prior release candidates for 2.0.0:

- ([#8084](https://github.com/EOSIO/eos/pull/8084)) Net plugin remove read delays - 2.0
- ([#8099](https://github.com/EOSIO/eos/pull/8099)) net_plugin remove sync w/peer check - 2.0
- ([#8120](https://github.com/EOSIO/eos/pull/8120)) Net plugin sync fix - 2.0
- ([#8229](https://github.com/EOSIO/eos/pull/8229)) Net plugin sync
- ([#8285](https://github.com/EOSIO/eos/pull/8285)) Net plugin handshake
- ([#8298](https://github.com/EOSIO/eos/pull/8298)) net_plugin lib sync
- ([#8303](https://github.com/EOSIO/eos/pull/8303)) net_plugin boost asio error handling
- ([#8305](https://github.com/EOSIO/eos/pull/8305)) net_plugin thread protection peer logging variables
- ([#8311](https://github.com/EOSIO/eos/pull/8311)) Fix race in fc::message_buffer and move message_buffer_tests to fc.
- ([#8307](https://github.com/EOSIO/eos/pull/8307)) reset the new handler

## Changes

### EOS VM: New High Performance WASM Runtimes ([#7974](https://github.com/EOSIO/eos/pull/7974), [#7975](https://github.com/EOSIO/eos/pull/7975))
Three new WASM runtimes are available in this release: EOS VM Interpreter, EOS VM Just In Time Compiler (JIT), and EOS VM Optimized Compiler.

EOS VM Interpreter is a low latency interpreter and is included to enable future support for smart contract debuggers. EOS VM JIT is a low latency single pass compiler for x86_64 platforms. To use EOS VM Interpreter or EOS VM JIT, set the `wasm-runtime` to either `eos-vm` or `eos-vm-jit` respectively

EOS VM Optimized Compiler is a high performance WASM _tier-up_ runtime available on the x86_64 Linux platform that works in conjunction with the configured baseline runtime (such as EOS VM JIT). When enabled via `eos-vm-oc-enable`, actions on a contract are initially dispatched to the baseline runtime while EOS VM Optimized Compiler does an optimized compilation in the background. Up to the configured `eos-vm-oc-compile-threads` compilations can be ongoing simultaneously. Once the background compilation is complete, actions on that contract will then be run with the optimized compiled code from then on. This optimized compile can take a handful of seconds but since it is performed in the background it does not cause delays. Optimized compilations are also saved to a new data file (`code_cache.bin` in the data directory) so when restarting nodeos there is no need to recompile previously compiled contracts.

None of the EOS VM runtimes require a replay nor activation of any consensus protocol upgrades. They can be switched between (including enabling and disabling EOS VM Optimized Compiler) at will.

At this time, **block producers should consider all running EOS VM JIT as the WASM runtime on their block producing nodes**. Other non-producing nodes should feel free to use the faster EOS VM Optimized Compiler runtime instead.

### Consensus Protocol Upgrades

Refer to this section on the [Upgrade Guide: Consensus Protocol Upgrades](../20_upgrade-guide/index.md#consensus-protocol-upgrades).

### Multi-threaded net_plugin
We have added multi-threading support to net_plugin. Almost all processing in the net_plugin (block propagation, transaction processing, block/transaction packing/unpacking etc.) are now handled by separate threads distinct from the main application thread. This significantly improves transaction processing and block processing performance on multi-producer EOSIO networks. The `net-threads` arg (defaults to 2) controls the number of worker threads in net_plugin thread pool.([#6845](https://github.com/EOSIO/eos/pull/6845), [#7598](https://github.com/EOSIO/eos/pull/7598), [#7392](https://github.com/EOSIO/eos/pull/7392), [#7786](https://github.com/EOSIO/eos/pull/7786) and related optimizations are available in: [#7686](https://github.com/EOSIO/eos/pull/7686), [#7785](https://github.com/EOSIO/eos/pull/7785), [#7721](https://github.com/EOSIO/eos/pull/7721), [#7825](https://github.com/EOSIO/eos/pull/7825), and [#7756](https://github.com/EOSIO/eos/pull/7756)).

### Chain API Enhancements ([#7530](https://github.com/EOSIO/eos/pull/7530))
The `uint128` and `int128` ABI types are now represented as decimal numbers rather than the old little-endian hexadecimal representation. This means that the JSON representation of table rows returned by the `get_table_rows` RPC will represent fields using this type differently than in prior versions. It also means that the `lower_bound` and `upper_bound` fields for `get_table_rows` RPC requests that search using a `uint128` secondary index will need to use the new decimal representation. This change makes the ABI serialization for `uint128` and `int128` ABI types consistent with [eosjs](https://github.com/EOSIO/eosjs) and [abieos](http://github.com/EOSIO/abieos).

The `get_table_rows` RPC when used with secondary index types like `sha256`, `i256`, and `ripemd160` had bugs that scrambled the bytes in the `lower_bound` and `upper_bound` input field. This release now fixes these issues allowing clients to properly search for tables rows using these index types.

A new field `next_key` has been added to the response of the `get_table_rows` RPC which represents the key of the next row (in the same format as `lower_bound` and `upper_bound`) that was not able to be returned in the response due to timeout limitations or the user-specified limit. The value of the `next_key` can be used as the `lower_bound` input for subsequent requests in order to retrieve a range of a large number of rows that could not be retrieved within just a single request.

## Upgrading from previous versions of EOSIO

Refer to this section on the [Upgrade Guide: Upgrading from previous versions of EOSIO](../20_upgrade-guide/index.md#upgrading-from-previous-versions-of-eosio).

## Deprecation and Removal Notices

### WAVM removed ([#8407](https://github.com/EOSIO/eos/pull/8407))

The WAVM WebAssembly runtime was deprecated several months ago with the release of [EOSIO v2.0.0-rc1](https://github.com/EOSIO/eos/releases/tag/v2.0.0-rc1) which introduced EOS VM. Now with the release of a stable version 2.0.0, EOS VM (JIT or Optimized Compiler variants) can be used instead of WAVM. So WAVM has now been removed as a WASM runtime option from nodeos. 

### Deprecation notice reminder

Please refer to the [Consolidated EOSIO Deprecations List](https://github.com/EOSIO/eos/issues/7597) for the currently active set of deprecation notices. 

## Other Changes

- ([#7247](https://github.com/EOSIO/eos/pull/7247)) Change default log level from debug to info. - develop
- ([#7238](https://github.com/EOSIO/eos/pull/7238)) Remove unused cpack bits; these are not being used
- ([#7248](https://github.com/EOSIO/eos/pull/7248)) Pass env to build script installs
- ([#6913](https://github.com/EOSIO/eos/pull/6913)) Block log util#6884
- ([#7249](https://github.com/EOSIO/eos/pull/7249)) Http configurable logging
- ([#7255](https://github.com/EOSIO/eos/pull/7255)) Move Test Metrics Code to Agent
- ([#7217](https://github.com/EOSIO/eos/pull/7217)) Created Universal Pipeline Configuration File
- ([#7264](https://github.com/EOSIO/eos/pull/7264)) use protocol-features-sync-nodes branch for LRT pipeline - develop
- ([#7207](https://github.com/EOSIO/eos/pull/7207)) Optimize mongodb plugin
- ([#7276](https://github.com/EOSIO/eos/pull/7276)) correct net_plugin's win32 check
- ([#7279](https://github.com/EOSIO/eos/pull/7279)) build unittests in c++17, not c++14
- ([#7282](https://github.com/EOSIO/eos/pull/7282)) Fix cleos REX help - develop
- ([#7284](https://github.com/EOSIO/eos/pull/7284)) remove boost asio string_view workaround
- ([#7286](https://github.com/EOSIO/eos/pull/7286)) chainbase uniqueness violation fixes
- ([#7287](https://github.com/EOSIO/eos/pull/7287)) restrict range of error codes that contracts are allowed to emit
- ([#7278](https://github.com/EOSIO/eos/pull/7278)) simplify openssl setup in cmakelists
- ([#7288](https://github.com/EOSIO/eos/pull/7288)) Changes for Boost 1_70_0
- ([#7285](https://github.com/EOSIO/eos/pull/7285)) Use of Mac Anka Fleet instead of iMac fleet
- ([#7293](https://github.com/EOSIO/eos/pull/7293)) Update EosioTester.cmake.in (develop)
- ([#7290](https://github.com/EOSIO/eos/pull/7290))  Block log util test
- ([#6845](https://github.com/EOSIO/eos/pull/6845)) Net plugin multithread
- ([#7295](https://github.com/EOSIO/eos/pull/7295)) Modify the pipeline control file for triggered builds
- ([#7305](https://github.com/EOSIO/eos/pull/7305)) Eliminating trigger to allow for community PR jobs to run again
- ([#7306](https://github.com/EOSIO/eos/pull/7306)) when unable to find mongo driver stop cmake
- ([#7309](https://github.com/EOSIO/eos/pull/7309)) add debug_mode to state history plugin - develop
- ([#7310](https://github.com/EOSIO/eos/pull/7310)) Switched to git checkout of commit + supporting forked repo
- ([#7291](https://github.com/EOSIO/eos/pull/7291)) add DB guard check for nodeos_under_min_avail_ram.py
- ([#7240](https://github.com/EOSIO/eos/pull/7240)) add signal tests
- ([#7315](https://github.com/EOSIO/eos/pull/7315)) Add REX balance info to cleos get account command
- ([#7304](https://github.com/EOSIO/eos/pull/7304)) transaction_metadata thread safety
- ([#7321](https://github.com/EOSIO/eos/pull/7321)) Only call init if system contract is loaded
- ([#7275](https://github.com/EOSIO/eos/pull/7275)) remove win32 from CMakeLists.txt
- ([#7322](https://github.com/EOSIO/eos/pull/7322)) Fix under min avail ram test
- ([#7327](https://github.com/EOSIO/eos/pull/7327)) fix block serialization order in fork_database::close - develop
- ([#7331](https://github.com/EOSIO/eos/pull/7331)) Use debug level logging for --verbose
- ([#7325](https://github.com/EOSIO/eos/pull/7325)) Look in both lib and lib64 for CMake modules when building EOSIO Tester
- ([#7337](https://github.com/EOSIO/eos/pull/7337)) correct signed mismatch warning in http_plugin
- ([#7348](https://github.com/EOSIO/eos/pull/7348)) Anka develop fix
- ([#7320](https://github.com/EOSIO/eos/pull/7320)) Add test for various chainbase objects which contain fields that require dynamic allocation
- ([#7350](https://github.com/EOSIO/eos/pull/7350)) correct signed mismatch warning in chain controller
- ([#7356](https://github.com/EOSIO/eos/pull/7356)) Fixes for Boost 1.70 to compile with our current CMake
- ([#7359](https://github.com/EOSIO/eos/pull/7359)) update to use boost 1.70
- ([#7341](https://github.com/EOSIO/eos/pull/7341)) Add Version Check for Package Builder
- ([#7367](https://github.com/EOSIO/eos/pull/7367)) Fix exception #
- ([#7342](https://github.com/EOSIO/eos/pull/7342)) Update to appbase max priority on main thread
- ([#7336](https://github.com/EOSIO/eos/pull/7336)) (softfloat sync) clean up strict-aliasing rules warnings
- ([#7371](https://github.com/EOSIO/eos/pull/7371)) Adds configuration for replay test pipeline
- ([#7370](https://github.com/EOSIO/eos/pull/7370)) Fix `-b` flag for `cleos get table` subcommand
- ([#7369](https://github.com/EOSIO/eos/pull/7369)) Add `boost/asio/io_context.hpp` header to `transaction_metadata.hpp` for branch `develop`
- ([#7385](https://github.com/EOSIO/eos/pull/7385)) Ship: port #7383 and #7384 to develop
- ([#7377](https://github.com/EOSIO/eos/pull/7377)) Allow aliases of variants in ABI
- ([#7316](https://github.com/EOSIO/eos/pull/7316)) Explicit name
- ([#7389](https://github.com/EOSIO/eos/pull/7389)) Fix develop merge
- ([#7379](https://github.com/EOSIO/eos/pull/7379)) transaction deadline cleanup
- ([#7380](https://github.com/EOSIO/eos/pull/7380)) Producer incoming-transaction-queue-size-mb
- ([#7391](https://github.com/EOSIO/eos/pull/7391)) Allow for EOS clone to be a submodule
- ([#7390](https://github.com/EOSIO/eos/pull/7390)) port db_modes_test to python
- ([#7399](https://github.com/EOSIO/eos/pull/7399)) throw error if trying to create non R1 key on SE or YubiHSM wallet
- ([#7394](https://github.com/EOSIO/eos/pull/7394)) (fc sync) static_variant improvements & fix certificate trust when trust settings is empty
- ([#7405](https://github.com/EOSIO/eos/pull/7405)) Centralize EOSIO Pipeline
- ([#7401](https://github.com/EOSIO/eos/pull/7401)) state database versioning
- ([#7392](https://github.com/EOSIO/eos/pull/7392)) Trx blk connections
- ([#7433](https://github.com/EOSIO/eos/pull/7433)) use imported targets for boost & cleanup fc/appbase/chainbase standalone usage
- ([#7434](https://github.com/EOSIO/eos/pull/7434)) (chainbase) don’t keep file mapping active when in heap/locked mode
- ([#7432](https://github.com/EOSIO/eos/pull/7432)) No need to start keosd for cleos command which doesn't need keosd
- ([#7430](https://github.com/EOSIO/eos/pull/7430)) Add option for cleos sign subcommand to ask keosd for signing
- ([#7425](https://github.com/EOSIO/eos/pull/7425)) txn json to file
- ([#7440](https://github.com/EOSIO/eos/pull/7440)) Fix #7436 SIGSEGV - develop
- ([#7461](https://github.com/EOSIO/eos/pull/7461)) Name txn_test_gen threads
- ([#7442](https://github.com/EOSIO/eos/pull/7442)) Enhance cleos error message when parsing JSON argument
- ([#7451](https://github.com/EOSIO/eos/pull/7451)) set initial costs for expensive parallel unit tests
- ([#7366](https://github.com/EOSIO/eos/pull/7366)) BATS bash tests for build scripts + various other improvements and fixes
- ([#7467](https://github.com/EOSIO/eos/pull/7467)) Pipeline Configuration File Update
- ([#7476](https://github.com/EOSIO/eos/pull/7476)) Various improvements from pull/7458
- ([#7488](https://github.com/EOSIO/eos/pull/7488)) Various BATS fixes to fix CI/CD
- ([#7489](https://github.com/EOSIO/eos/pull/7489)) Fix Incorrectly Resolved and Untested Merge Conflicts in Pipeline Configuration File
- ([#7474](https://github.com/EOSIO/eos/pull/7474)) fix copy_bin() for win32 builds
- ([#7478](https://github.com/EOSIO/eos/pull/7478)) remove stray SIGUSR1
- ([#7475](https://github.com/EOSIO/eos/pull/7475)) guard unix socket support in http_plugin depending on platform support
- ([#7492](https://github.com/EOSIO/eos/pull/7492)) Enabled helpers for unpinned builds.
- ([#7482](https://github.com/EOSIO/eos/pull/7482)) Let delete-all-blocks option to only remove the contents instead of the directory itself
- ([#7502](https://github.com/EOSIO/eos/pull/7502)) [develop] Versioned images (prep for hashing)
- ([#7507](https://github.com/EOSIO/eos/pull/7507)) use create_directories in initialize_protocol_features - develop
- ([#7484](https://github.com/EOSIO/eos/pull/7484)) return zero exit status for nodeos version, help, fixed reversible, and extracted genesis
- ([#7468](https://github.com/EOSIO/eos/pull/7468)) Versioning library
- ([#7486](https://github.com/EOSIO/eos/pull/7486)) [develop] Ensure we're in repo root
- ([#7515](https://github.com/EOSIO/eos/pull/7515)) Custom path support for eosio installation.
- ([#7516](https://github.com/EOSIO/eos/pull/7516)) on supported platforms build with system clang by default
- ([#7519](https://github.com/EOSIO/eos/pull/7519)) [develop] Removed lrt from pipeline.jsonc
- ([#7525](https://github.com/EOSIO/eos/pull/7525)) [develop] Readlink quick fix and BATS test fixes
- ([#7532](https://github.com/EOSIO/eos/pull/7532)) [develop] BASE IMAGE Fixes
- ([#7535](https://github.com/EOSIO/eos/pull/7535)) Add -j option to print JSON format for cleos get currency balance.
- ([#7537](https://github.com/EOSIO/eos/pull/7537)) connection via listen needs to start in connecting mode
- ([#7497](https://github.com/EOSIO/eos/pull/7497)) properly add single quotes for parameter with spaces in logs output
- ([#7544](https://github.com/EOSIO/eos/pull/7544)) restore usage of devtoolset-8 on centos7
- ([#7547](https://github.com/EOSIO/eos/pull/7547)) Sighup logging
- ([#7421](https://github.com/EOSIO/eos/pull/7421)) WebAuthn key and signature support
- ([#7572](https://github.com/EOSIO/eos/pull/7572)) [develop] Don't create mongo folders unless ENABLE_MONGO is true
- ([#7576](https://github.com/EOSIO/eos/pull/7576)) [develop] Better found/not found messages for clarity
- ([#7545](https://github.com/EOSIO/eos/pull/7545)) add nodiscard attribute to tester's push_action
- ([#7581](https://github.com/EOSIO/eos/pull/7581)) Update to fc with logger fix
- ([#7558](https://github.com/EOSIO/eos/pull/7558)) [develop] Ability to set *_DIR on CLI
- ([#7553](https://github.com/EOSIO/eos/pull/7553)) [develop] CMAKE version check before dependencies are installed
- ([#7584](https://github.com/EOSIO/eos/pull/7584)) fix hash<>::result_type deprecation spam
- ([#7588](https://github.com/EOSIO/eos/pull/7588)) [develop] Various BATS test fixes
- ([#7578](https://github.com/EOSIO/eos/pull/7578)) [develop] -i support for relative paths
- ([#7449](https://github.com/EOSIO/eos/pull/7449)) add accurate checktime timer for macOS
- ([#7591](https://github.com/EOSIO/eos/pull/7591)) [develop] SUDO_COMMAND -> NEW_SUDO_COMMAND (base fixed)
- ([#7594](https://github.com/EOSIO/eos/pull/7594)) [develop] Install location fix
- ([#7586](https://github.com/EOSIO/eos/pull/7586)) Modify transaction_ack to process bcast_transaction and rejected_transaction correctly
- ([#7585](https://github.com/EOSIO/eos/pull/7585)) Enhance cleos to enable new RPC send_transaction
- ([#7599](https://github.com/EOSIO/eos/pull/7599)) Use updated sync nodes for sync tests
- ([#7608](https://github.com/EOSIO/eos/pull/7608)) indicate in brew bottle mojave is required
- ([#7615](https://github.com/EOSIO/eos/pull/7615)) Change hardcoded currency symbol in testnet.template into a variable
- ([#7610](https://github.com/EOSIO/eos/pull/7610)) use explicit billing for unapplied and deferred transactions in tester - develop
- ([#7621](https://github.com/EOSIO/eos/pull/7621)) Call resolve on connection strand
- ([#7623](https://github.com/EOSIO/eos/pull/7623)) additional wasm unittests around max depth
- ([#7607](https://github.com/EOSIO/eos/pull/7607)) Improve nodeos make-index speeds
- ([#7598](https://github.com/EOSIO/eos/pull/7598)) Net plugin block id notification
- ([#7624](https://github.com/EOSIO/eos/pull/7624)) bios-boot-tutorial.py: bugfix, SYS hardcoded instead of using command…
- ([#7632](https://github.com/EOSIO/eos/pull/7632)) [develop] issues/7627: Install script missing !
- ([#7634](https://github.com/EOSIO/eos/pull/7634)) fix fc::temp_directory usage in tester
- ([#7642](https://github.com/EOSIO/eos/pull/7642)) remove stale warning about dirty metadata
- ([#7640](https://github.com/EOSIO/eos/pull/7640)) fix win32 build of eosio-blocklog
- ([#7643](https://github.com/EOSIO/eos/pull/7643)) remove unused dlfcn.h include; troublesome for win32
- ([#7645](https://github.com/EOSIO/eos/pull/7645)) add eosio-blocklog to base install component
- ([#7651](https://github.com/EOSIO/eos/pull/7651)) Port #7619 to develop
- ([#7652](https://github.com/EOSIO/eos/pull/7652)) remove raise() in keosd in favor of simple appbase quit (de-posix it)
- ([#7453](https://github.com/EOSIO/eos/pull/7453)) Refactor unapplied transaction queue
- ([#7657](https://github.com/EOSIO/eos/pull/7657)) (chainbase sync) print name of DB causing failure condition & win32 fixes
- ([#7663](https://github.com/EOSIO/eos/pull/7663)) Fix path error in cleos set code/abi
- ([#7633](https://github.com/EOSIO/eos/pull/7633)) Small optimization to move more trx processing off application thread
- ([#7662](https://github.com/EOSIO/eos/pull/7662)) fix fork resolve in special case
- ([#7667](https://github.com/EOSIO/eos/pull/7667)) fix 7600 double confirm after changing sign key
- ([#7625](https://github.com/EOSIO/eos/pull/7625)) Fix flaky tests - mainly net_plugin fixes
- ([#7672](https://github.com/EOSIO/eos/pull/7672)) Fix memory leak
- ([#7676](https://github.com/EOSIO/eos/pull/7676)) Remove unused code
- ([#7677](https://github.com/EOSIO/eos/pull/7677)) Commas go outside the quotes...
- ([#7675](https://github.com/EOSIO/eos/pull/7675)) wasm unit test with an imported function as start function
- ([#7678](https://github.com/EOSIO/eos/pull/7678)) Issue 3516 fix
- ([#7404](https://github.com/EOSIO/eos/pull/7404)) wtmsig block production
- ([#7686](https://github.com/EOSIO/eos/pull/7686)) Unapplied transaction queue performance
- ([#7685](https://github.com/EOSIO/eos/pull/7685)) Integration Test descriptions and timeout fix
- ([#7691](https://github.com/EOSIO/eos/pull/7691)) Fix nodeos 1.8.x to > 1.7.x peering issue (allowed-connection not equal to "any")
- ([#7250](https://github.com/EOSIO/eos/pull/7250)) wabt: reduce redundant memset
- ([#7702](https://github.com/EOSIO/eos/pull/7702)) fix producer_plugin watermark tracking - develop
- ([#7477](https://github.com/EOSIO/eos/pull/7477)) Fix abi_serializer to encode optional non-built_in types
- ([#7703](https://github.com/EOSIO/eos/pull/7703)) return flat_multimap from transaction::validate_and_extract_extensions
- ([#7720](https://github.com/EOSIO/eos/pull/7720)) Fix bug to make sed -i work properly on Mac
- ([#7716](https://github.com/EOSIO/eos/pull/7716)) [TRAVIS POC] develop Support passing in JOBS for docker/kube multi-tenancy
- ([#7707](https://github.com/EOSIO/eos/pull/7707)) add softfloat only injection mode
- ([#7725](https://github.com/EOSIO/eos/pull/7725)) Fix increment in test
- ([#7729](https://github.com/EOSIO/eos/pull/7729)) Fix db_modes_test
- ([#7734](https://github.com/EOSIO/eos/pull/7734)) Fix db exhaustion
- ([#7487](https://github.com/EOSIO/eos/pull/7487)) Enable extended_asset to be encoded from array
- ([#7736](https://github.com/EOSIO/eos/pull/7736)) unit test ensuring that OOB table init allowed on set code; fails on action
- ([#7744](https://github.com/EOSIO/eos/pull/7744)) (appbase) update to get non-option fix & unique_ptr tweak
- ([#7746](https://github.com/EOSIO/eos/pull/7746)) (chainbase sync) fix build with boost 1.71
- ([#7721](https://github.com/EOSIO/eos/pull/7721)) Improve signature recovery
- ([#7757](https://github.com/EOSIO/eos/pull/7757)) remove stale license headers
- ([#7756](https://github.com/EOSIO/eos/pull/7756)) block_log performance improvement, and misc.
- ([#7654](https://github.com/EOSIO/eos/pull/7654)) exclusively use timer for checktime
- ([#7763](https://github.com/EOSIO/eos/pull/7763)) Use fc::cfile instead of std::fstream for state_history
- ([#7770](https://github.com/EOSIO/eos/pull/7770)) Net plugin sync fix
- ([#7717](https://github.com/EOSIO/eos/pull/7717)) Support for v2 snapshots with pending producer schedules
- ([#7795](https://github.com/EOSIO/eos/pull/7795)) Hardcode initial eosio ABI: #7794
- ([#7792](https://github.com/EOSIO/eos/pull/7792)) Restore default logging if logging.json is removed when SIGHUP.
- ([#7791](https://github.com/EOSIO/eos/pull/7791)) Producer plugin
- ([#7786](https://github.com/EOSIO/eos/pull/7786)) Remove redundant work from net plugin
- ([#7785](https://github.com/EOSIO/eos/pull/7785)) Optimize block log usage
- ([#7812](https://github.com/EOSIO/eos/pull/7812)) Add IMPORTANT file and update README - develop
- ([#7820](https://github.com/EOSIO/eos/pull/7820)) rename IMPORTANT to IMPORTANT.md - develop
- ([#7809](https://github.com/EOSIO/eos/pull/7809)) Correct cpu_usage calculation when more than one signature
- ([#7803](https://github.com/EOSIO/eos/pull/7803)) callback support for checktime timer expiry
- ([#7838](https://github.com/EOSIO/eos/pull/7838)) Bandwidth - develop
- ([#7845](https://github.com/EOSIO/eos/pull/7845)) Deprecate network version match - develop
- ([#7700](https://github.com/EOSIO/eos/pull/7700)) [develop] Travis CI + Buildkite 3.0
- ([#7825](https://github.com/EOSIO/eos/pull/7825)) apply_block optimization
- ([#7849](https://github.com/EOSIO/eos/pull/7849)) Increase Contracts Builder Timeout + Fix $SKIP_MAC
- ([#7854](https://github.com/EOSIO/eos/pull/7854)) Net plugin sync
- ([#7860](https://github.com/EOSIO/eos/pull/7860)) [develop] Ensure release flag is added to all builds.
- ([#7873](https://github.com/EOSIO/eos/pull/7873)) [develop] Mac Builder Boost Fix
- ([#7868](https://github.com/EOSIO/eos/pull/7868)) cleos get actions
- ([#7774](https://github.com/EOSIO/eos/pull/7774)) update WAVM to be compatible with LLVM 7 through 9
- ([#7864](https://github.com/EOSIO/eos/pull/7864)) Add output of build info on nodeos startup
- ([#7881](https://github.com/EOSIO/eos/pull/7881)) [develop] Ensure Artfacts Upload on Failed Tests
- ([#7886](https://github.com/EOSIO/eos/pull/7886)) promote read-only disablement log from net_plugin to warn level
- ([#7887](https://github.com/EOSIO/eos/pull/7887)) print unix socket path when there is an error starting unix socket server
- ([#7889](https://github.com/EOSIO/eos/pull/7889)) Update docker builder tag
- ([#7891](https://github.com/EOSIO/eos/pull/7891)) Fix exit crash - develop
- ([#7877](https://github.com/EOSIO/eos/pull/7877)) Create Release Build Test
- ([#7883](https://github.com/EOSIO/eos/pull/7883)) Add Support for eosio-test-stability Pipeline
- ([#7903](https://github.com/EOSIO/eos/pull/7903)) adds support for builder priority queues
- ([#7853](https://github.com/EOSIO/eos/pull/7853)) change behavior of recover_key to better support variable length keys
- ([#7901](https://github.com/EOSIO/eos/pull/7901)) [develop] Add Trigger for LRTs and Multiversion Tests Post PR
- ([#7914](https://github.com/EOSIO/eos/pull/7914)) [Develop] Forked PR fix
- ([#7923](https://github.com/EOSIO/eos/pull/7923)) return error when attempting to remove key from YubiHSM wallet
- ([#7910](https://github.com/EOSIO/eos/pull/7910)) [Develop] Updated anka plugin, added failover for registries, and added sleep fix for git clone/networking bug
- ([#7926](https://github.com/EOSIO/eos/pull/7926)) Better error check in test
- ([#7919](https://github.com/EOSIO/eos/pull/7919)) decouple wavm runtime from being required & initial support for platform specific wasm runtimes
- ([#7927](https://github.com/EOSIO/eos/pull/7927)) Fix Release Build Type for macOS on Travis CI
- ([#7931](https://github.com/EOSIO/eos/pull/7931)) Fix intermittent crash on exit when port already in use - develop
- ([#7930](https://github.com/EOSIO/eos/pull/7930)) use -fdiagnostics-color=always even for clang
- ([#7933](https://github.com/EOSIO/eos/pull/7933)) [Develop] Support all BK/Travis cases in Submodule Regression Script
- ([#7943](https://github.com/EOSIO/eos/pull/7943)) Change eosio-launcher enable-gelf-logging argument default to false.
- ([#7946](https://github.com/EOSIO/eos/pull/7946)) Forked chain test error statement - develop
- ([#7948](https://github.com/EOSIO/eos/pull/7948)) net_plugin correctly handle unknown_block_exception - develop
- ([#7954](https://github.com/EOSIO/eos/pull/7954)) remove bad semicolon (in unused but compiled code)
- ([#7952](https://github.com/EOSIO/eos/pull/7952)) Unable to Create Block Log Index #7865
- ([#7953](https://github.com/EOSIO/eos/pull/7953)) Refactor producer plugin start_block - develop
- ([#7841](https://github.com/EOSIO/eos/pull/7841)) 7646 chain id in blog
- ([#7958](https://github.com/EOSIO/eos/pull/7958)) [develop] Fix Mac builds on Travis
- ([#7962](https://github.com/EOSIO/eos/pull/7962)) set immutable chain_id during construction of controller
- ([#7957](https://github.com/EOSIO/eos/pull/7957)) Upgrade to Boost 1.71.0
- ([#7971](https://github.com/EOSIO/eos/pull/7971)) Net plugin unexpected block - develop
- ([#7967](https://github.com/EOSIO/eos/pull/7967)) support unix socket HTTP server for nodeos
- ([#7947](https://github.com/EOSIO/eos/pull/7947)) Function body code size test
- ([#7955](https://github.com/EOSIO/eos/pull/7955)) EOSIO WASM Spec tests
- ([#7978](https://github.com/EOSIO/eos/pull/7978)) use the LLVM 7 library provided by SCL on CentOS7
- ([#7983](https://github.com/EOSIO/eos/pull/7983)) port consolidated security fixes for 1.8.4 to develop; add unit tests associated with consolidated security fixes for 1.8.1
- ([#7986](https://github.com/EOSIO/eos/pull/7986)) Remove unnecessary comment
- ([#7985](https://github.com/EOSIO/eos/pull/7985)) more bug fixes with chain_id in state changes
- ([#7974](https://github.com/EOSIO/eos/pull/7974)) Experimental/wb2 jit
- ([#7989](https://github.com/EOSIO/eos/pull/7989)) Correct designator order for field of get_table_rows_params
- ([#7992](https://github.com/EOSIO/eos/pull/7992)) move wasm_allocator from wasm_interface to controller
- ([#7995](https://github.com/EOSIO/eos/pull/7995)) new timeout to handle when two jobs on the same host are maxing their…
- ([#7993](https://github.com/EOSIO/eos/pull/7993)) update eos-vm to latest develop, fix issues with instantiation limit …
- ([#7991](https://github.com/EOSIO/eos/pull/7991)) missing block log chain id unit tests
- ([#8001](https://github.com/EOSIO/eos/pull/8001)) Net plugin trx progress - develop
- ([#8003](https://github.com/EOSIO/eos/pull/8003)) update eos-vm ref
- ([#7988](https://github.com/EOSIO/eos/pull/7988)) Net plugin version match
- ([#8004](https://github.com/EOSIO/eos/pull/8004)) bump version
- ([#7975](https://github.com/EOSIO/eos/pull/7975)) EOS-VM Optimized Compiler
- ([#8007](https://github.com/EOSIO/eos/pull/8007)) disallow WAVM with EOS-VM OC
- ([#8010](https://github.com/EOSIO/eos/pull/8010)) Change log level of index write
- ([#8009](https://github.com/EOSIO/eos/pull/8009)) pending incoming order on subjective failure
- ([#8013](https://github.com/EOSIO/eos/pull/8013)) ensure eos-vm-oc headers get installed
- ([#8008](https://github.com/EOSIO/eos/pull/8008)) Increase stability of nodeos_under_min_avail_ram.py - develop
- ([#8015](https://github.com/EOSIO/eos/pull/8015)) two fixes for eosio tester cmake modules
- ([#8019](https://github.com/EOSIO/eos/pull/8019)) [Develop] Change submodule script to see stderr for git commands
- ([#8014](https://github.com/EOSIO/eos/pull/8014)) Retain persisted trx until expired on speculative nodes
- ([#8024](https://github.com/EOSIO/eos/pull/8024)) Add optional ability to disable WASM Spec Tests
- ([#8023](https://github.com/EOSIO/eos/pull/8023)) Make subjective_cpu_leeway a config option
- ([#8025](https://github.com/EOSIO/eos/pull/8025)) Fix build script LLVM symlinking
- ([#8026](https://github.com/EOSIO/eos/pull/8026)) update EOS VM Optimized Compiler naming convention
- ([#8012](https://github.com/EOSIO/eos/pull/8012)) 7939 trim block log v3 support
- ([#8029](https://github.com/EOSIO/eos/pull/8029)) update eos-vm ref and install eos-vm license
- ([#8033](https://github.com/EOSIO/eos/pull/8033)) net_plugin better error for unknown block
- ([#8034](https://github.com/EOSIO/eos/pull/8034)) EOS VM OC license updates
- ([#8042](https://github.com/EOSIO/eos/pull/8042)) [2.0.x] dockerhub | eosio/producer -> eosio/ci
- ([#8050](https://github.com/EOSIO/eos/pull/8050)) Add greylist limit - v2.0.x
- ([#8060](https://github.com/EOSIO/eos/pull/8060)) #8054: fix commas in ship abi
- ([#8072](https://github.com/EOSIO/eos/pull/8072))  nodeos & keosd version reporting - 2.0
- ([#8071](https://github.com/EOSIO/eos/pull/8071)) Update cleos to support new producer schedule - 2.0
- ([#8070](https://github.com/EOSIO/eos/pull/8070)) don't rebuild llvm unnecessarily during pinned builds - 2.0
- ([#8074](https://github.com/EOSIO/eos/pull/8074)) [2.0.x] Upgrade mac anka template to 10.14.6
- ([#8076](https://github.com/EOSIO/eos/pull/8076)) Handle cases where version_* not specified in CMakeLists.txt - 2.0
- ([#8088](https://github.com/EOSIO/eos/pull/8088)) [2.0.x] Linux build fleet update
- ([#8091](https://github.com/EOSIO/eos/pull/8091)) report block extensions_type contents in RPC and eosio-blocklog tool - 2.0
- ([#8105](https://github.com/EOSIO/eos/pull/8105)) Modify --print-default-config to exit with success - 2.0
- ([#8113](https://github.com/EOSIO/eos/pull/8113)) [2.0.x] WASM Spec Test Step in CI
- ([#8114](https://github.com/EOSIO/eos/pull/8114)) [2.0.x] Mac OSX steps need a min of 1 hour
- ([#8127](https://github.com/EOSIO/eos/pull/8127)) [2.0.x] Move the ensure step into the build step, eliminating the need for templaters
- ([#8144](https://github.com/EOSIO/eos/pull/8144)) fix pinned builds on fresh macOS install - 2.0
- ([#8149](https://github.com/EOSIO/eos/pull/8149)) [2.0.x] CI platform directories
- ([#8155](https://github.com/EOSIO/eos/pull/8155)) Post State history callback as medium priority - 2.0
- ([#8173](https://github.com/EOSIO/eos/pull/8173)) ensure GMP is always dynamically linked - 2.0
- ([#8175](https://github.com/EOSIO/eos/pull/8175)) [2.0.x] Unpinned and WASM test fixes
- ([#8168](https://github.com/EOSIO/eos/pull/8168)) add harden flags to cicd & pinned builds - 2.0
- ([#8180](https://github.com/EOSIO/eos/pull/8180)) [2.0.x] 10 second sleep to address heavy usage wait-network bug in Anka
- ([#8192](https://github.com/EOSIO/eos/pull/8192)) Reduce logging - 2.0
- ([#8367](https://github.com/EOSIO/eos/pull/8367)) Add Sync from Genesis Test
- ([#8363](https://github.com/EOSIO/eos/pull/8363)) Fix linking OpenSSL (branch `release/2.0.x`)
- ([#8383](https://github.com/EOSIO/eos/pull/8383)) Escape BUILDKITE_COMMIT to generate tag properly
- ([#8385](https://github.com/EOSIO/eos/pull/8385)) Propagate exceptions out push_block - 2.0
- ([#8391](https://github.com/EOSIO/eos/pull/8391)) Add eosio-resume-from-state Test
- ([#8393](https://github.com/EOSIO/eos/pull/8393)) Make multiversion protocol test conditional.
- ([#8402](https://github.com/EOSIO/eos/pull/8402)) fix EOS VM OC monitor thread name - 2.0
- ([#8406](https://github.com/EOSIO/eos/pull/8406)) [2.0.x] Modified Amazon and Centos to use yum install ccache
- ([#8414](https://github.com/EOSIO/eos/pull/8414)) Add better logging of exceptions in emit - 2.0
- ([#8328](https://github.com/EOSIO/eos/pull/8328)) Fix bios boot python script due to 2.0.x changes
- ([#8293](https://github.com/EOSIO/eos/pull/8293)) Add nodeos/cleos/keosd docs from develop, update README
- ([#8425](https://github.com/EOSIO/eos/pull/8425)) fix discovery of openssl in tester cmake when OPENSSL_ROOT_DIR not set - 2.0

## Thanks!

Special thanks to the community contributors that submitted patches for this release:
- @UMU618
- @conr2d
- @YordanPavlov
- @baegjae
- @olexiybuyanskyy
- @spartucus
- @rdewilder
