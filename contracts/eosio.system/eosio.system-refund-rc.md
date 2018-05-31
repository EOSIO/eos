# Action - `{{ refund }}`

### Description

The intent of the `{{ refund }}` action is to return previously unstaked tokens to an account after the unstaking period has elapsed. 

### Input and Input Type

The `{{ refund }}` action requires the following `input` and `input type`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ refund }}` | `{{ ownerVar }}` | `{{ account_name }}` |


As an authorized party I {{ signer }} wish to have the unstaked tokens of {{ ownerVar }} returned.
