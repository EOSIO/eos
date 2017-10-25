Welcome to the EOS.IO Documentation
-----------------------------------

@note This documentation is in progress and subject to change due to rapid development. Please report inaccuracies identified to the [EOS.IO Developer Telegram Group](https://t.me/joinchat/EaEnSUPktgfoI-XPfMYtcQ)

## EOS.IO
 - [EOS.IO Official Website](https://www.eos.io/)
 - [Github](https://github.com/eosio)
 - [Blog](https://steemit.com/@eosio)
 - [Roadmap](https://github.com/EOSIO/Documentation/blob/master/Roadmap.md/)
 - [Telegram](https://www.eos.io/chat/)
 - [White Paper](https://github.com/EOSIO/Documentation/blob/master/TechnicalWhitePaper.md/)

## Environment Setup
- Automatic Build Scripts
 	- [Ubuntu 16.10](https://github.com/EOSIO/eos#autoubuntu)
	- [macOS Sierra 10.12.6](https://github.com/EOSIO/eos#automac)
- [Manual Installation of Dependencies](https://github.com/EOSIO/eos#manualdep)
	- [Clean Install Ubuntu 16.10](https://github.com/EOSIO/eos#ubuntu)
	- [Clean Install macOS Sierra 10.12.6](https://github.com/EOSIO/eos#macos)
- [Docker](https://github.com/EOSIO/eos/tree/master/Docker)

## End User
- Wallet Creation and Management
	- [Create a wallet](https://eosio.github.io/eos/group__eosc.html#createwallet)
	- [Import Key to Wallet](https://eosio.github.io/eos/group__eosc.html#importkey)
	- [Locking and Unlocking Wallet](https://eosio.github.io/eos/group__eosc.html#lockwallets)
	- [Create an Account](https://eosio.github.io/eos/group__eosc.html#createaccount)
	- [Transfer EOS](https://eosio.github.io/eos/group__eosc.html#transfereos)
	- [Getting Transaction](https://eosio.github.io/eos/group__eosc.html#gettingtransaction)

## Smart Contract Developers
- [Introduction to Contract Development Tutorial](https://eosio.github.io/eos/md_contracts_eoslib_tutorial.html)
- [How to Deploy Smart Contracts](https://github.com/EOSIO/eos#smartcontracts)
- [RPC Interface](https://eosio.github.io/eos/group__eosiorpc.html)
- [API Reference](https://eosio.github.io/eos/group__contractdev.html)
	- @ref database - APIs that store and retreive data on the blockchainEOS.IO organizes data according to the following broad structure
	- @ref mathapi - Defines common math function
	- @ref messageapi - Define API for querying message properties
	- @ref consoleapi - Enables applications to log/print text messages
	- @ref tokens - Defines the ABI for interfacing with standard-compatible token messages and database tables
	- @ref types - Specifies typedefs and aliases
	- @ref transactionapi - Define API for sending transactions and inline messages
	- @ref systemapi - 	Define API for interating with system level intrinsics
