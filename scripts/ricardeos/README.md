# Purpose
The `ricardeos.py` imports or exports recardian contracts to and from a contracts abi

## Import Example
`$ python3 ricardeos.py import /path/to/sorce-contract.abi /path/to/new-smart-contract-abi.abi`

Running this will scan the directory of the abi for all rc.md files and add them to their respective actions.  All files with a path format of *clause*-rc.md will be added to the ricardian_clauses section.  You can provide the same name for the source and new smart contract abi, the script will prompt you before overwriting.

The script will also notify the user of any actions that the script cannot find rc.md files for.

## Export Example
`$ python3 ricardeos.py export /path/to/sorce-contract.abi`

Running this will dump the contents of all ricardian contracts:

Actions will be exported in the following format: `<contract-name>-<action-name>-rc.md`

Clauses will be exported in the following format: `<contract-name>-clause-<clause-id>-rc.md`

If a file already exists the user will be asked if they wish to overwrite the file
