# Purpose
The `abi_to_rc.py` script processes a contract's .abi file in order to generate an overview Ricardian Contract and a Ricardian Contract for each action. The overview Ricardian Contract provides a description of the contract's purpose and also specifies the contract's action(s), input(s), and input type(s). The action Ricardian Contract provides a description of the action's purpose and also specifies the action's input(s), and input type(s).

## How to run
`$ python abi_to_rc.py /path/to/smart-contract.abi`

## Example
`$ python abi_to_rc.py ../../contracts/currency/currency.abi`

## Results
For the example above, `abi_to_rc.py` should generate output files that have the following names: `currency-rc.md`, `currency-transfer-rc.md`, `currency-issue-rc.md`, `currency-create-rc.md`.

## Notes
Be sure to have `abi_to_rc.py`, `rc-overview-template.md`, and `rc-action-template.md` in the same folder.
