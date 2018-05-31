# Action - `{{ canceldelay }}`

### Description

The `{{ canceldelay }}` action cancels an existing delayed transaction.

### Inputs and Input Types

The `{{ canceldelay }}` action requires the following `inputs` and `input types`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ canceldelay }}` | `{{ canceling_authVar }}`<br/>`{{ trx_idVar }}` | `{{ permission_level }}`<br/>`{{ transaction_id_type }}` |

As an authorized party I {{ signer }} wish to invoke the authority of {{ canceling_authVar }} to cancel the transaction with ID {{ trx_idVar }}.
