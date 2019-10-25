# rem.contracts

## Version : 1.0.0-x

The design of the EOSIO blockchain calls for a number of smart contracts that are run at a privileged permission level in order to support functions such as block producer registration and voting, token staking for CPU and network bandwidth, RAM purchasing, multi-sig, etc.  These smart contracts are referred to as the bios, system, msig, wrap (formerly known as sudo) and token contracts.

This repository contains examples of these privileged contracts that are useful when deploying, managing, and/or using an EOSIO blockchain.  They are provided for reference purposes:

   * [rem.bios](https://github.com/Remmeauth/remprotocol/tree/develop/contracts/contracts/rem.bios)
   * [rem.system](https://github.com/Remmeauth/remprotocol/tree/develop/contracts/contracts/rem.system)
   * [rem.msig](https://github.com/Remmeauth/remprotocol/tree/develop/contracts/contracts/rem.msig)
   * [rem.wrap](https://github.com/Remmeauth/remprotocol/tree/develop/contracts/contracts/rem.wrap)

The following unprivileged contract(s) are also part of the system.
   * [rem.token](https://github.com/Remmeauth/remchain/tree/develop/contracts/contracts/rem.token)

Dependencies:
* [eosio.cdt v1.7.x](https://github.com/EOSIO/eosio.cdt/releases/tag/v1.7.0-rc1)
* [eosio v2.0.x](https://github.com/EOSIO/eos/releases/tag/v2.0.0-rc1) (optional dependency only needed to build unit tests)

To build contracts alone:
1. Ensure an appropriate version of eosio.cdt is installed. Installing eosio.cdt from binaries is sufficient.
2. Run the `build.sh` script in the top directory to build all the contracts.

To build the contracts and unit tests:
1. Ensure an appropriate version of eosio.cdt is installed. Installing eosio.cdt from binaries is sufficient.
2. Ensure an appropriate version of eosio has been built from source and installed. Installing eosio from binaries is not sufficient.
3. Run the `build.sh` script in the top directory with the `-t` flag to build all the contracts and the unit tests for these contracts.

After build:
* If the build was configured to also build unit tests, the unit tests executable is placed in the _build/tests_ folder and is named __unit_test__.
* The contracts (both `.wasm` and `.abi` files) are built into their corresponding _build/contracts/\<contract name\>_ folder.
* Finally, simply use __remcli__ to _set contract_ by pointing to the previously mentioned directory for the specific contract.

## Contributing

[Contributing Guide](./CONTRIBUTING.md)

[Code of Conduct](./CONTRIBUTING.md#conduct)

## License

[MIT](./LICENSE)

The included icons are provided under the same terms as the software and accompanying documentation, the MIT License.  We welcome contributions from the artistically-inclined members of the community, and if you do send us alternative icons, then you are providing them under those same terms.

## Important

See [LICENSE](./LICENSE) for copyright and license terms.

All repositories and other materials are provided subject to the terms of this [IMPORTANT](./IMPORTANT.md) notice and you must familiarize yourself with its terms.  The notice contains important information, limitations and restrictions relating to our software, publications, trademarks, third-party resources, and forward-looking statements.  By accessing any of our repositories and other materials, you accept and agree to the terms of the notice.
