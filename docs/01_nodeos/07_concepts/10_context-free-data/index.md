---
content_title: Context-Free Data (CFD)
link_text: Context-Free Data
---

The immutable nature of the blockchain allows data to be stored securely while also enforcing the integrity of such data. However, this benefit also complicates the removal of non-essential data from the blockchain. Consequently, EOSIO blockchains contain a special section within the transaction, called the *context-free data*, which would allow the potential removal of such data, as long as the contents are free from previous contexts or dependencies. More importantly, such removal must be performed safely without compromising the integrity of the blockchain.

## Concept
The goal of context-free data is to allow certain blockchain applications the option to store non-essential information within a transaction. Some examples of context-free data include:

* Secondary blockchain data that is transient or temporary in nature.
* Short-term, non-critical data associated with a transaction message.
* User comments made to an online article stored on the blockchain.

In general, any data that is not vital for the operation and integrity of the blockchain is probably a good candidate for context-free data. Also, data that might be mandated to be removed from the blockchain in order to comply with certain laws and regulations.
