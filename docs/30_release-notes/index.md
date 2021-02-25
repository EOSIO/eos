---
content_title: EOSIO v2.0.10 Release Notes
---

This release contains security, stability, and miscellaneous fixes.

## Security bug fixes

### Consolidated Security Fixes for v2.0.10 ([#10091](https://github.com/EOSIO/eos/pull/10091))
- Fix issue with account query db that could result in incorrect data or hung processes 
- Implement a Subjective CPU billing system that helps P2P and API nodes better respond to extreme network congestion

Note: These security fixes are relevant to all nodes on EOSIO blockchain networks.

### Notes on Subjective CPU Billing

This system consists of two primary features: a subjective (node local) view of spent CPU resources that are not yet accounted for by the blockchain that allows individual nodes to predict what resource consumption will be and, a subjective penalty system to offset work done in service of erroneous or malicious transactions. 

The subjective view of CPU resources will synchronize with the resources present in the blockchain as it discovers the true CPU billing for transactions it has already accounted for.

The system will also accumulate CPU resources spent on failing transactions that will not be relayed in a decaying "subjective penalty" which can protect the individual nodes from abusive actors while remaining tolerant to occasional mistakes. 

Subjective billing defaults to active and can be disabled with the `disable-subjective-billing` configuration in `config.ini` or on the command line. 

## Stability bug fixes
- ([#9985](https://github.com/EOSIO/eos/pull/9985)) EPE-389 net_plugin stall during head catchup - merge release/2.0.x

## Other changes
- ([#9894](https://github.com/EOSIO/eos/pull/9894)) EOS VM OC: Support LLVM 11 - 2.0
- ([#9911](https://github.com/EOSIO/eos/pull/9911)) add step to the pipeline to build and push to dockerhub on release br…
- ([#9944](https://github.com/EOSIO/eos/pull/9944)) Create eosio-debug-build Pipeline
- ([#9969](https://github.com/EOSIO/eos/pull/9969)) Updating name for the new Docker hub repo EOSIO instead EOS
- ([#9971](https://github.com/EOSIO/eos/pull/9971)) Fix pinned builds error due to obsolete LLVM repo
- ([#10015](https://github.com/EOSIO/eos/pull/10015)) [release 2.0.x] Fix LRT triggers
- ([#10026](https://github.com/EOSIO/eos/pull/10026)) EPE-165: Improve logic for unlinkable blocks while sync'ing
- ([#10047](https://github.com/EOSIO/eos/pull/10047)) Reduce Docker Hub Manifest Queries
- ([#10088](https://github.com/EOSIO/eos/pull/10088)) [release 2.0.x] Specify boost version for unpinned MacOS 10.14 builds

## Documentation
- ([#10011](https://github.com/EOSIO/eos/pull/10011)) [docs] Update various cleos how-tos and fix index - 2.0
