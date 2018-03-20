# Purpose
The `abi_to_rc.py` script processes a contract's .abi file in order to generate an overview Ricardian Contract and a Ricardian Contract for each action. The overview Ricardian Contract provides a description of the contract's purpose and also specifies the contract's action(s), input(s), and input type(s). The action Ricardian Contract provides a description of the action's purpose and also specifies the action's input(s), and input type(s).

## How to run
`python abi_to_rc.py ${{smart-contract.abi}} rc-overview-template.md rc-action-template.md`

## Example
`python abi_to_rc.py currency.abi rc-overview-template.md rc-action-template.md`

## Results
For the example above, if all goes well, `abi_to_rc.py` should generate output files that have the following names: `currency-rc.md`, `currency-issue-rc.md`, `currency-transfer-rc.md`.

## Notes
`abi_to_rc.py` was written in Python 3. Be sure to have `abi_to_rc.py`, `currency.abi` (or whichever `.abi` file that is being used), `rc-overview-template.md`, and `rc-action-template.md` all in the same folder.
