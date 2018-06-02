# Action - `{{ unregprod }}`

### Description

The `{{ unregprod }}` action unregisters a previously registered block producer candidate.

### Input and Input Type

The `{{ unregprod }}` action requires the following `input` and `input type`:

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ unregprod }}` | `{{ producerVar }}` | `{{ account_name }}` |

As an authorized party I {{ signer }} wish to unregister the block producer candidate {{ producerVar }}, rendering that candidate no longer able to receive votes.
