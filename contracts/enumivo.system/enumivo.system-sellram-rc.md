# Action - `{{ sellram }}`

### Description

The `{{ sellram }}` action sells unused RAM for tokens.

### Inputs and Input Types

The `{{ sellram }}` action requires the following `inputs` and `input types`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ sellram }}` | `{{ accountVar }}`<br/>`{{ bytesVar }}` | `{{ account_name }}`<br/>`{{ uint64 }}` |

As an authorized party I {{ signer }} wish to sell {{ bytesVar }} of RAM from account {{ accountVar }}. 
