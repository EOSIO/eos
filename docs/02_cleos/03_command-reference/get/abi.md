## Description
Retrieves the ABI for an account

## Positional Parameters
- `name` _TEXT_ - The name of the account whose abi should be retrieved

## Options
- `-f,--file` _TEXT_ - The name of the file to save the contract .abi to instead of writing to console

## Examples
Retrieve and save abi for apifiny.token contract

```shell
$ clapifiny get abi apifiny.token -f apifiny.token.abi

saving abi to apifiny.token.abi
```
