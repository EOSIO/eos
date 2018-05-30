# Action - `{{ setcode }}`

This Contract is legally binding and can be used in the event of a dispute. 

### Description

The intention of the `{{ setcode }}` action is to load a smart contract into memory and make it available for execution.

### Inputs and Input Types

The `{{ setcode }}` action requires the following `inputs` and `input types`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ setcode }}` | `{{ accountVar }}`<br/>`{{ vmtypeVar }}`<br/>`{{ vmversionVar }}`<br/>`{{ codeVar }}` | `{{ account_name }}`<br/>`{{ uint8 }}`<br/>`{{ uint8 }}`<br/>`{{ bytes }}` |

As an authorized party I {{ signer }} wish to store in  memory owned by {{ accountVar }} the code {{ codeVar }} which shall use VM Type {{ vmtypeVar }} version {{ vmversionVar }}.

All disputes arising from this contract shall be resolved in the EOS Core Arbitration Forum. 
