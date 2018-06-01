# Action - `{{ newaccount }}`

### Description

The `{{ newaccount }}` action creates a new account.

### Inputs and Input Types

The `{{ newaccount }}` action requires the following `inputs` and `input types`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ newaccount }}` | `{{ creatorVar }}`<br/>`{{ nameVar }}`<br/>`{{ ownerVar }}`<br/>`{{ activeVar }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ authority }}`<br/>`{{ authority }}` |

As an authorized party I {{ signer }} wish to exercise the authority of {{ creatorVar }} to create a new account on this system named {{ nameVar }} such that the new account's owner public key shall be {{ ownerVar }} and the active public key shall be {{ activeVar }}.
