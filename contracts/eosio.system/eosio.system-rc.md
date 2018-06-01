# Smart Contract - `{{ eosio.system }}`

This is an overview of the actions for the `{{ eosio.system }}` smart contract. This Contract is legally binding and can be used in the event of a dispute. Disputes shall be settled through the standard arbitration process established by EOS.IO.

### Description

The `{{ eosio.system }}` contract...

### Actions, Inputs and Input Types

The table below contains the `actions`, `inputs` and `input types` for the `{{ eosio.system }}` contract.

| Action | Input | Input Type |
|:--|:--|:--|
| `{{ newaccount }}` | `{{ creator }}`<br/>`{{ name }}`<br/>`{{ owner }}`<br/>`{{ active }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ authority }}`<br/>`{{ authority }}` |
| `{{ setcode }}` | `{{ account }}`<br/>`{{ vmtype }}`<br/>`{{ vmversion }}`<br/>`{{ code }}` | `{{ account_name }}`<br/>`{{ uint8 }}`<br/>`{{ uint8 }}`<br/>`{{ bytes }}` |
| `{{ setabi }}` | `{{ account }}`<br/>`{{ abi }}` | `{{ account_name }}`<br/>`{{ bytes }}` |
| `{{ updateauth }}` | `{{ account }}`<br/>`{{ permission }}`<br/>`{{ parent }}`<br/>`{{ auth }}` | `{{ account_name }}`<br/>`{{ permission_name }}`<br/>`{{ permission_name }}`<br/>`{{ authority }}` |
| `{{ deleteauth }}` | `{{ account }}`<br/>`{{ permission }}` | `{{ account_name }}`<br/>`{{ permission_name }}` |
| `{{ linkauth }}` | `{{ account }}`<br/>`{{ code }}`<br/>`{{ type }}`<br/>`{{ requirement }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ action_name }}`<br/>`{{ permission_name }}` |
| `{{ unlinkauth }}` | `{{ account }}`<br/>`{{ code }}`<br/>`{{ type }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ action_name }}` |
| `{{ canceldelay }}` | `{{ canceling_auth }}`<br/>`{{ trx_id }}` | `{{ permission_level }}`<br/>`{{ transaction_id_type }}` |
| `{{ onerror }}` | `{{ sender_id }}`<br/>`{{ sent_trx }}` | `{{ uint128 }}`<br/>`{{ bytes }}` |
| `{{ buyrambytes }}` | `{{ payer }}`<br/>`{{ receiver }}`<br/>`{{ bytes }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ uint32 }}` |
| `{{ buyram }}` | `{{ payer }}`<br/>`{{ receiver }}`<br/>`{{ quant }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ asset }}` |
| `{{ sellram }}` | `{{ account }}`<br/>`{{ bytes }}` | `{{ account_name }}`<br/>`{{ uint64 }}` |
| `{{ delegatebw }}` | `{{ from }}`<br/>`{{ receiver }}`<br/>`{{ stake_net_quantity }}`<br/>`{{ stake_cpu_quantity }}`<br/>`{{ transfer }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ asset }}`<br/>`{{ asset }}`<br/>`{{ bool }}` |
| `{{ undelegatebw }}` | `{{ from }}`<br/>`{{ receiver }}`<br/>`{{ unstake_net_quantity }}`<br/>`{{ unstake_cpu_quantity }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ asset }}`<br/>`{{ asset }}` |
| `{{ refund }}` | `{{ owner }}` | `{{ account_name }}` |
| `{{ regproducer }}` | `{{ producer }}`<br/>`{{ producer_key }}`<br/>`{{ url }}`<br/>`{{ location }}` | `{{ account_name }}`<br/>`{{ public_key }}`<br/>`{{ string }}`<br/>`{{ uint16 }}` |
| `{{ setram }}` | `{{ max_ram_size }}` | `{{ uint64 }}` |
| `{{ bidname }}` | `{{ bidder }}`<br/>`{{ newname }}`<br/>`{{ bid }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ asset }}` |
| `{{ unregprod }}` | `{{ producer }}` | `{{ account_name }}` |
| `{{ regproxy }}` | `{{ proxy }}`<br/>`{{ isproxy }}` | `{{ account_name }}`<br/>`{{ bool }}` |
| `{{ voteproducer }}` | `{{ voter }}`<br/>`{{ proxy }}`<br/>`{{ producers }}` | `{{ account_name }}`<br/>`{{ account_name }}`<br/>`{{ account_name[] }}` |
| `{{ claimrewards }}` | `{{ owner }}` | `{{ account_name }}` |
| `{{ setpriv }}` | `{{ account }}`<br/>`{{ is_priv }}` | `{{ account_name }}`<br/>`{{ int8 }}` |
| `{{ setalimits }}` | `{{ account }}`<br/>`{{ ram_bytes }}`<br/>`{{ net_weight }}`<br/>`{{ cpu_weight }}` | `{{ account_name }}`<br/>`{{ int64 }}`<br/>`{{ int64 }}`<br/>`{{ int64 }}` |
| `{{ setglimits }}` | `{{ cpu_usec_per_period }}` | `{{ int64 }}` |
| `{{ setprods }}` | `{{ schedule }}` | `{{ producer_key[] }}` |
| `{{ reqauth }}` | `{{ from }}` | `{{ account_name }}` |