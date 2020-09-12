---
content_title: EOSIO 2.0 Upgrade Guide
---

This section contains important instructions for node operators and other EOSIO stakeholders to transition an EOSIO network successfully through an EOSIO version or protocol upgrade.

## Consensus Protocol Upgrades
EOSIO v2.0.x introduces two new consensus protocol upgrade features (or just protocol features for short):

1. `WEBAUTHN_KEY`: Adds support for WebAuthn keys and signatures ([#7421](https://github.com/EOSIO/eos/pull/7421))
2. `WTMSIG_BLOCK_SIGNATURES`: Adds support for weighted threshold multi-signature (WTMsig) authorities for block production ([#7404](https://github.com/EOSIO/eos/pull/7404), [#8002](https://github.com/EOSIO/eos/pull/8002), [#8021](https://github.com/EOSIO/eos/pull/8021))

Both of these features require activation through the privileged `preactivate_feature` intrinsic. This is typically facilitated by executing the `eosio::activate` system contract action (requires system contract [v1.7.0](https://github.com/EOSIO/eosio.contracts/releases/tag/v1.7.0) or later) via the `eosio.msig` contract after getting supermajority block producer approval. However, while these protocol features can be activated through coordination of the block producers alone, **it is still critical to give sufficient time for all nodes to upgrade to v2.0.x before activating any of the above two protocol features**. Activation of any of these two protocol features will immediately fork out any nodes that do not support the protocol features (e.g. any nodes running a version of nodeos earlier than v2.0.x).

Both of the above protocol features only add new behavior to the EOSIO blockchain. They are not expected to change existing behavior in a significant way that would cause existing contracts to break. Existing applications and block explorers are also unlikely to break upon activation of these protocol features. However, these new features enable new data structures to be utilized on-chain which applications and especially block explorers need to be prepared to handle. For example, activation of the `WEBAUTHN_KEY` protocol feature means that an `eosio::newaccount` or `eosio::updateauth` action could now include WebAuthn public keys as part of the provided `authority`. So, an application dealing with these actions (e.g. a general authenticator for EOSIO transactions) will need to be prepared to support the serialization of WebAuthn public keys appearing in a place where a `public_key` ABI type is expected. Another example is how the activation of the `WTMSIG_BLOCK_SIGNATURES` protocol feature causes the `new_producers` field of a block header to always remain empty, even when a proposed producer schedule gets promoted to pending; upon activation of the `WTMSIG_BLOCK_SIGNATURES` protocol feature, the pending schedule data is instead stored (when present) within the `header_extensions` of the block header. So, block explorers expecting to show these pending producer schedules to users will need to update their code accordingly. As usual, test networks provide a great environment to try existing contracts, applications, services, etc. with the changes of the above protocol features to determine what breaks and what needs to be updated. 

For more details on the `WEBAUTH_KEY` protocol feature, see the section titled [WebAuthn Support](#webauthn-support-7421). For more details on the `WTMSIG_BLOCK_SIGNATURES` protocol feature, see the section titled [Weighted Threshold Multi-signature (WTMsig) Block Signing Authorities](#weighted-threshold-multi-signature-block-signing-authorities-7403).

### WebAuthn Support ([#7421](https://github.com/EOSIO/eos/pull/7421))
WebAuthn support within EOSIO is now available for developers to begin testing WebAuthn based transaction signing in their EOSIO applications. Private EOSIO chains can [start](https://github.com/EOSIO/eosio-webauthn-example-app) using WebAuthn based signatures with this release. We also anticipate releasing additional demos which will attempt to outline how Dapp authors can leverage WebAuthn for contract actions meriting higher levels of security as a second factor authorization mechanism. 

### Weighted Threshold Multi-Signature Block Signing Authorities ([#7403](https://github.com/EOSIO/eos/issues/7403))
Block producers must be able to provide high availability for their core service of running the blockchain (aka producing blocks). A common approach to achieve this is redundant infrastructure that efficiently maintains block production, in the event of a hardware malfunction or networking issues. Weighted Threshold Multi-Signature Block Production is the first of many features that seek to provide block producers with a complete high availability solution.

Current consensus rules require exactly one cryptographic block signing key per block producer.  This key, whether stored on disk and loaded via software or protected with a hardware wallet, represents a single-point-of-failure for the operations of a block producer. If that key is lost or access to the hardware module that contains it is temporarily unavailable, the block producer has no choice but to drop blocks, impacting the whole network’s throughput.

To improve the security and scalability of block production, weighted threshold multi-signature (WTMsig) block signing authorities provides a permission layer that allows for multiple block signing keys in a flexible scheme that will enable redundant block signing infrastructure to exist without sharing any sensitive data.

An updated version of the system contract is required to enable block producers to use WTMsig block signing authorities. Version [v1.9.0-rc1](https://github.com/EOSIO/eosio.contracts/releases/tag/v1.9.0-rc1) of eosio.contracts adds a new `regproducer2` action to the eosio.system contract which enables a block producer candidate to register a WTMsig block signing authority. (That version of the system contract can only be deployed on an EOSIO chain that has activated the `WTMSIG_BLOCK_SIGNATURES` protocol feature.) More details are available at: [#7404](https://github.com/EOSIO/eos/pull/7404), [#8002](https://github.com/EOSIO/eos/pull/8002), [#8021](https://github.com/EOSIO/eos/pull/8021).

## Upgrading from previous versions of EOSIO

EOSIO v2.0.x has made changes to the state database that prevent it from working with existing state databases in v1.8.x and earlier. So upgrading from earlier versions of EOSIO requires importing the state from a portable snapshot.

Furthermore, the changes to the state database necessitate a version bump in the portable snapshots generated by EOSIO v2.0.x from v2 to v3. However, unlike the upgrade of portable snapshot versions in EOSIO v1.8.x from v1 to v2, EOSIO v2.0.x is able to import v2 portable snapshots. This means that it is not necessary to replay the blockchain from genesis to upgrade from EOSIO v1.8.x to v2.0.x. (One exception is if the operator of the node is using the deprecated history_plugin and wishes to preserve that history.)

Finally, EOSIO v2.0.x has also upgraded the version of the irreversible blocks log to v3. However, older versions of the blocks log are still supported, so there is no need to do anything special to handle existing blocks log files.   

### Upgrading from v1.8.x

If the node uses the deprecated history_plugin (and the operator of the node wishes to preserve this history), the only option to upgrade is to replay the blockchain from genesis. 

Users of the state_history_plugin (SHiP) do not need to replay from genesis because the state history logs are portable and contain versioned data structures within. However, upgrading a node that uses state history without a full replay means that the state history log will contain a mix of versions for any upgrade types. For example, the changes in v2.0.x modify the `global_property_object` in the database state and so the state history log could contain a `global_property_object_v0` type (for the part of the history before the local node upgrade) and a `global_property_object_v1` type (for the part of the history after the local node upgrade). This should not cause problems for any history fillers that have been updated to support both versions of the type. However, operators should be aware that this can lead to the log file contents being slightly different across different nodes even if they all start and stop at the same blocks and have not enabled `trace-history-debug-mode`. (Replaying a v2.0.x node with state_history_plugin enabled from genesis would guarantee that the state history logs do not contain the `global_property_object_v0` type.)

The following instructions should be followed to upgrade nodeos from v1.8.x to v2.0.x without a full replay (after making appropriate backup copies):
1. Obtain a compatible portable snapshot of the state. A v2 or v3 portable snapshot can be downloaded from a trusted source. Alternatively, one can use an existing v1.8.x node to generate a v2 portable snapshot by launching nodeos with `--read-mode=irreversible --plugin=eosio::producer_api_plugin` command-line options and then using the `/v1/producer/create_snapshot` RPC endpoint to generate a portable snapshot (e.g. run the command `curl -X POST http:/127.0.0.1:8888/v1/producer/create_snapshot -d '{}' | jq`).
2. Shut down nodeos and delete the `blocks/reversible` and `state` sub-directories within the data directory.
3. Launch nodeos v2.0.x from the generated snapshot using the `--snapshot` command line option and give it time to import the snapshot while starting up (this could take several minutes). (Subsequent launches of nodeos should not use the `--snapshot` option.)

### Upgrading from v2.0.0-rc1, v2.0.0-rc2, or v2.0.0-rc3

Node operators should consider upgrading v2.0.0-rc1, v2.0.0-rc2, and v2.0.0-rc3 nodes to v2.0.0 as soon as possible. They can follow normal upgrade procedures for the upgrade. There should be no need to do a replay or import from a snapshot.
