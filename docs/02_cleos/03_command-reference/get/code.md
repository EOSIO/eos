## Description
Retrieves the code and ABI for an account

## Positional Parameters
- `name` _TEXT_ - The name of the account whose code should be retrieved
## Options
- `-c,--code` _TEXT_ - The name of the file to save the contract _.wast_ to
- `-a,--abi` _TEXT_ - The name of the file to save the contract _.abi_ to
- `--wasm` Save contract as wasm
## Examples
Simply output the hash of apifiny.token contract

```shell
$ clapifiny get code apifiny.token
code hash: f675e7aeffbf562c033acfaf33eadff255dacb90d002db51c7ad7cbf057eb791
```
Retrieve and save abi for apifiny.token contract

```shell
$ clapifiny get code apifiny.token -a apifiny.token.abi
code hash: f675e7aeffbf562c033acfaf33eadff255dacb90d002db51c7ad7cbf057eb791
saving abi to apifiny.token.abi
```
Retrieve and save wast code for apifiny.token contract

```shell
$ clapifiny get code apifiny.token -c apifiny.token.wast
code hash: f675e7aeffbf562c033acfaf33eadff255dacb90d002db51c7ad7cbf057eb791
saving wast to apifiny.token.wast
```
