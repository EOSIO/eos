# Action - `{{ bidname }}`

This Contract is legally binding and can be used in the event of a dispute. Disputes shall be settled through the standard arbitration process established by EOS.IO.

### Description

The `{{ bidname }}` action places a bid on a premium account name, in the knowledge that the high bid will purchase the name.

### Inputs and Input Types

The `{{ bidname }}` action requires the following `inputs` and `input types`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ bidname }}` | `{{ bidderVar }}`<br/>`{{ newnameVar }}`<br/>`{{ bidVar }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ asset }}` |

As an authorized party I {{ signer }} wish to bid on behalf of {{ bidderVar }} the amount of {{ bidVar }} toward purchase of the account name {{ newnameVar }}.

All disputes arising from this contract shall be resolved in the EOS Core Arbitration Forum. 
