Welcome to the EOS.io Documentation
-----------------------------------

@note This documentation is in progress and subject to change due present rapid development. Please report inaccuracies identified to the [EOS developer telegram group](https://t.me/joinchat/EaEnSUPktgfoI-XPfMYtcQ)

## EOS
 - [EOS.IO Official Website](https://www.eos.io/)
 - [Blog](https://steemit.com/@eosio)
 - [Roadmap](https://github.com/EOSIO/Documentation/blob/master/Roadmap.md/)
 - [Telegram](https://www.eos.io/chat/)
 - [White Paper](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md/)

## Building EOS
- [Environment Setup]()
	- Automatic Build Script [Ubuntu 16.10](https://github.com/EOSIO/eos#autoubuntu) [macOS Sierra 10.12.6](https://github.com/EOSIO/eos#automac)
	- Manual Build [Ubuntu 16.10](https://github.com/EOSIO/eos#ubuntu) [macOS Sierra 10.12.6](https://github.com/EOSIO/eos#macos)
	- [Docker](https://github.com/EOSIO/eos/tree/master/Docker)

## End User
- Wallet Creation and Management
	- [Create a wallet](https://github.com/EOSIO/eos/tree/docs-432#generaluse-createwallet)
	- [Generate keypair](https://github.com/EOSIO/eos/tree/docs-432#generaluse-generatekeys)
	- [Create an Account](https://github.com/EOSIO/eos/tree/docs-432#generaluse-createaccount)
	- [Check if Account Exists](https://github.com/EOSIO/eos/tree/docs-432#generaluse-accountexists)
	- [Importing a Wallet](https://github.com/EOSIO/eos/tree/docs-432#generaluse-walletimport)

## Smart Contract Developers
- [Introduction](https://eosio.github.io/eos/group__contractdev.html)
- [Smart Contracts](https://github.com/EOSIO/eos#accountssmartcontracts)
	- [Hello World ](https://eosio.github.io/eos/md_contracts_eoslib_tutorial.html)
	- [Example Contracts](https://github.com/EOSIO/eos#example-smart-contracts)
	- [Deploying Contracts](https://github.com/EOSIO/eos#upload-sample-contract-to-blockchain)
- [Benchmarking]()
- [RPC Interface](https://eosio.github.io/eos/group__eosiorpc.html)
- API Reference
	- @ref databaseapi - APIs that store and retreive data on the blockchainEOS.IO organizes data according to the following broad structure
	- @ref mathapi - Defines common math function
	- @ref messageapi - Define API for querying message properties
	- @ref consoleapi - Enables applications to log/print text messages
	- @ref tokens - Defines the ABI for interfacing with standard-compatible token messages and database tables
	- @ref types - Specifies typedefs and aliases
	- @ref transactionapi - Define API for sending transactions and inline messages
	- @ref systemapi - 	Define API for interating with system level intrinsics