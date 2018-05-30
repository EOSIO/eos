# Action - `{{ canceldelay }}`

This Contract is legally binding and can be used in the event of a dispute. 

### Description

The `{{ canceldelay }}` action cancels an existing delayed transaction.

### Inputs and Input Types

The `{{ canceldelay }}` action requires the following `inputs` and `input types`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ canceldelay }}` | `{{ canceling_authVar }}`<br/>`{{ trx_idVar }}` | `{{ permission_level }}`<br/>`{{ transaction_id_type }}` |

As an authorized party I {{ signer }} wish to invoke the authority of {{ canceling_authVar }} to cancel the transaction with ID {{ trx_idVar }}.

All disputes arising from this contract shall be resolved in the EOS Core Arbitration Forum. 
