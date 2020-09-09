---
content_title: Context-Free Data (CFD)
link_text: Context-Free Data
---

## Overview
The immutable nature of the blockchain allows data to be stored securely while also enforcing the integrity of such data. However, this benefit also complicates the removal of non-essential data from the blockchain. Consequently, EOSIO blockchains contain a special section within the transaction, called the *context-free data*. As its name implies, data stored in the context-free data section is considered free of previous contexts or dependencies, which makes their potential removal possible. More importantly, such removal can be performed safely without compromising the integrity of the blockchain.

## Concept
The goal of context-free data is to allow blockchain applications the option to store non-essential information within a transaction. Some examples of context-free data include:

* Secondary blockchain data that is transient or temporary in nature
* Short-term, non-critical data associated with a transaction message
* User comments made to an online article stored on the blockchain

In general, any data that is not vital for the operation and integrity of the blockchain may be stored as context-free data. It may also be used to comply with regional laws and regulations concerning data usage and personal information.
