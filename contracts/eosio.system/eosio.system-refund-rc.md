# Action - `{{ refund }}`

This Contract is legally binding and can be used in the event of a dispute. Disputes shall be settled through the standard arbitration process established by EOS.IO.

### Description

The intent of the `{{ refund }}` action is to return previously unstaked tokens to an account after the unstaking period has elapsed. 

### Input and Input Type

The `{{ refund }}` action requires the following `input` and `input type`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ refund }}` | `{{ ownerVar }}` | `{{ account_name }}` |


As an authorized party I {{ signer }} wish to have the unstaked tokens of {{ ownerVar }} returned.

All disputes arising from this contract shall be resolved in the EOS Core Arbitration Forum. 
