---
content_title: Private Access Control
link_text: Private Access Control
---

## Overview

The *Private Access Control* or *Security Group* feature provides a two-layer security solution designed for blockchain administrators to control which participants can access and use a private EOSIO network. The first security layer is designed to enforce TLS connections in the peer-to-peer network protocol; so only those participants with a properly signed certificate are able to establish a TLS connection and communicate securely with other peers. The second security layer, which is optional, is designed to bring the concept of a *privacy group* or *security group*; so that if enabled, only those participants in the group are allowed to access data from the private network. Therefore, the first layer controls “connection” access while the second layer manages “data” access. Participants are considered logical entities with a designated EOSIO name and may host multiple nodes, potentially deployed in [Block Vault](../../03_plugins/blockvault_client_plugin/index.md) mode.

## Concept

Most businesses need to keep data private for regulatory compliance, customer privacy, or both. Since blockchain solutions have a history of public data and open architecture, businesses need certain privacy and security guarantees before they can adopt blockchain technology. The *Private Access Control* feature aims to bring EOSIO closer to the Enterprise by providing a general business security solution that can be used across a variety of [Use Cases](#use-cases).

[[info | Access controls via consensus]]
| Along with the inherent features of the EOSIO blockchain, the *Private Access Control* feature is designed to achieve security, robustness, and coordination through EOSIO consensus. The protocol feature can be enabled and participants added or removed from a *security group* only after the block that contains the corresponding transaction is approved by a supermajority of producer participants and the block becomes [irreversible](https://developers.eos.io/welcome/latest/glossary/index/#irreversible-block).

## Requirements

The EOSIO software supports the *Private Access Control* feature by meeting the following requirements:
- The SECURITY_GROUP protocol feature must be activated first via EOSIO consensus.
- Once activated, only nodes with valid signed credentials are allowed to connect.
- Valid nodes must establish TLS connections to communicate securely with other peers.
- Once connected, all peer-to-peer communications must be encrypted - TLS satisfies this.
- Optionally, a *security group* can be used to manage participant data access to the network:
  * If at least one participant is added to the group, data access control is enabled.
  * If no participants are added to the group, data access control is disabled.
  * When data access control is disabled, all participants in the group are allowed data access.
  * When data access control is enabled, only those participants in the group are allowed data access.

The secure connection requirements are met by establishing TLS connections among the participant nodes. This is considered sufficient from a security standpoint since the peer-to-peer communications are encrypted. The optional data access control requirements are met through the *security group* which adds further data access control on a per-participant basis. Adding and removing participants within the *security group* as well as additional housekeeping operations are implemented through EOSIO WASM host functions, which can be invoked from wrapper actions within custom smart contracts.

[[info | Technical note]]
| Removing a participant from the *security group* means that a peer belonging to that participant will not get any data from the *security group* despite having a valid certificate. However, it will still be able to receive peer-to-peer (p2p) handshake data such as head number, lib number, and other non-essential data.

## Interface

The *Private Access Control* feature provides a [C++ security_group API](https://developers.eos.io/manuals/eosio.cdt/latest/security__group_8hpp) for data access control on a per-participant basis. The API is composed of C++ wrappers to EOSIO WASM host functions. Smart contract developers are encouraged to create custom wrapper actions over this interface and implement additional business logic according to their application requirements. A sample [security_group_test](https://github.com/EOSIO/eos/blob/develop/unittests/test-contracts/security_group_test/security_group_test.cpp) smart contract is provided for reference.

### C++ security_group API

The C++ security_group API exposes the following functions to be used within smart contracts. You can add or remove participants, check existence within the *security group*, or get a list of the current participants:

* [`add_security_group_participants()`](https://developers.eos.io/manuals/eosio.cdt/latest/namespaceeosio_1_1internal__use__do__not__use#function-add_security_group_participants)
* [`remove_security_group_participants()`](https://developers.eos.io/manuals/eosio.cdt/latest/namespaceeosio_1_1internal__use__do__not__use#function-remove_security_group_participants)
* [`in_active_security_group()`](https://developers.eos.io/manuals/eosio.cdt/latest/namespaceeosio_1_1internal__use__do__not__use#function-in_active_security_group)
* [`get_active_security_group()`](https://developers.eos.io/manuals/eosio.cdt/latest/namespaceeosio_1_1internal__use__do__not__use#function-get_active_security_group)

Note that adding or removing participants within the *security group* requires a privileged account. Also, note that these two actions are "proposed" for approval via EOSIO consensus. Finally, checking for existence of participants in the *security group* is all or nothing: if there is at least one specified participant that is not in the *security group*, the check returns `false`.

For more information visit the [C++ security_group API](https://developers.eos.io/manuals/eosio.cdt/latest/security__group_8hpp) reference.

## Examples

The examples below showcase how to enable the SECURITY_GROUP protocol feature as well as adding and removing participants from the *security group*:
* [Private Access Control Tutorial](05_tutorial.md)
<!--
* [How to add participants to security group](10_how-to-add-participants-to-security-group.md)
* [How to remove participants to security group](15_how-to-remove-participants-to-security-group.md)
-->

## Use Cases

Below are some of the potential use cases for *Private Access Control*, along with applicable business privacy requirements:

* Consortium Solutions:
  - Control on-chain user access and control via smart contracts.
  - Allow storage and transmission of encrypted data among consortium members.
  - Maintain a synced registry of approved vendors with all participants of the chain, with transparency on when a record is added or edited.
  - Transaction recording and tracking using hash references of private chain data to a parent chain of consortium members.

* Supply Chain Solutions:
  - Private messaging between Clients to exchange confidential messages between trading partners by leveraging [EPCIS](https://www.gs1.org/standards/epcis) technology and standards.
  - Blockchain as a shared, immutable ledger to register the proof of the authenticity of transactions and execute smart contracts. The blockchain will enforce business rules, such as only one company can have legal ownership of a serialized unit at a given time.
  - Zero-Knowledge Succinct Non-Interactive Argument of Knowledge (zk-SNARKs) or another method of enhancing privacy to ensure no business data is revealed.

* Financial Solutions:
  - Ability to have permission based controls on a private chain so that user read/write permissions can be controlled at the smart contract level.
  - To be able to add/remove/edit users and organizations within a private chain.

[[info | Private Blockchains]]
| Private EOSIO blockchains can benefit the most from the *Private Access Control* feature. Their controlled environment allows to create a *security group*, add, and remove participants with ease.
