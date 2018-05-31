# Action - `{{ claimrewards }}`

### Description

The `{{ claimrewards }}` action allows a block producer (active or standby) to claim the system rewards due them for producing blocks and receiving votes.

### Input and Input Type

The `{{ claimrewards }}` action requires the following `input` and `input type`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ claimrewards }}` | `{{ ownerVar }}` | `{{ account_name }}` |

As an authorized party I {{ signer }} wish to have the rewards earned by {{ ownerVar }} deposited into their (my)  account.
