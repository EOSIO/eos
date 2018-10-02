extern const char* const state_history_plugin_abi = R"({
    "version": "eosio::abi/1.1",
    "structs": [
        {
            "name": "get_state_request_v0", "fields": []
        },
        {
            "name": "get_state_result_v0", "fields": [
                { "name": "last_block_num", "type": "uint32" }
            ]
        },
        {
            "name": "get_block_request_v0", "fields": [
                { "name": "block_num", "type": "uint32" }
            ]
        },
        {
            "name": "get_block_result_v0", "fields": [
                { "name": "block_num", "type": "uint32" },
                { "name": "found", "type": "bool" },
                { "name": "deltas", "type": "bytes" }
            ]
        },
        {
            "name": "row", "fields": [
                { "name": "id", "type": "uint64" },
                { "name": "data", "type": "bytes" }
            ]
        },
        {
            "name": "table_delta_v0", "fields": [
                { "name": "name", "type": "string" },
                { "name": "rows", "type": "row[]" },
                { "name": "removed", "type": "uint64[]" }
            ]
        },
        {
            "name": "account_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "name", "name": "name" },
                { "type": "uint8", "name": "vm_type" },
                { "type": "uint8", "name": "vm_version" },
                { "type": "bool", "name": "privileged" },
                { "type": "time_point", "name": " last_code_update" },
                { "type": "checksum256", "name": "code_version" },
                { "type": "block_timestamp_type", "name": "creation_date" },
                { "type": "bytes", "name": "code" },
                { "type": "bytes", "name": "abi" }
            ]
        },
        {
            "name": "account_sequence_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "name", "name": "name" },
                { "type": "uint64", "name": "recv_sequence" },
                { "type": "uint64", "name": "auth_sequence" },
                { "type": "uint64", "name": "code_sequence" },
                { "type": "uint64", "name": "abi_sequence" }
            ]
        },
        {
            "name": "table_id_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "name", "name": "payer" },
                { "type": "uint32", "name": "count" }
            ]
        },
        {
            "name": "key_value_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "t_id" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "bytes", "name": "value" }
            ]
        },
        {
            "name": "index64_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "t_id" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "uint64", "name": "secondary_key" }
            ]
        },
        {
            "name": "index128_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "t_id" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "uint128", "name": "secondary_key" }
            ]
        },
        {
            "name": "index256_key", "fields": [
                { "type": "uint128", "name": "a0" },
                { "type": "uint128", "name": "a1" }
            ]
        },
        {
            "name": "index256_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "t_id" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "index256_key", "name": "secondary_key" }
            ]
        },
        {
            "name": "index_double_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "t_id" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "float64", "name": "secondary_key" }
            ]
        },
        {
            "name": "index_long_double_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "t_id" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "float128", "name": "secondary_key" }
            ]
        },
        {
            "name": "producer_key_v0", "fields": [
                { "type": "name", "name": "producer_name" },
                { "type": "public_key", "name": "block_signing_key" }
            ]
        },
        {
            "name": "shared_producer_schedule_type_v0", "fields": [
                { "type": "uint32", "name": "version" },
                { "type": "producer_key[]", "name": "producers" }
            ]
        },
        {
            "name": "chain_config_v0", "fields": [
                { "type": "uint64", "name": "max_block_net_usage" },
                { "type": "uint32", "name": "target_block_net_usage_pct" },
                { "type": "uint32", "name": "max_transaction_net_usage" },
                { "type": "uint32", "name": "base_per_transaction_net_usage" },
                { "type": "uint32", "name": "net_usage_leeway" },
                { "type": "uint32", "name": "context_free_discount_net_usage_num" },
                { "type": "uint32", "name": "context_free_discount_net_usage_den" },
                { "type": "uint32", "name": "max_block_cpu_usage" },
                { "type": "uint32", "name": "target_block_cpu_usage_pct" },
                { "type": "uint32", "name": "max_transaction_cpu_usage" },
                { "type": "uint32", "name": "min_transaction_cpu_usage" },
                { "type": "uint32", "name": "max_transaction_lifetime" },
                { "type": "uint32", "name": "deferred_trx_expiration_window" },
                { "type": "uint32", "name": "max_transaction_delay" },
                { "type": "uint32", "name": "max_inline_action_size" },
                { "type": "uint16", "name": "max_inline_action_depth" },
                { "type": "uint16", "name": "max_authority_depth" }
            ]
        },
        {
            "name": "global_property_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint32?", "name": "proposed_schedule_block_num" },
                { "type": "shared_producer_schedule_type", "name": "proposed_schedule" },
                { "type": "chain_config", "name": "configuration" }
            ]
        },
        {
            "name": "dynamic_global_property_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "global_action_sequence" }
            ]
        },
        {
            "name": "block_summary_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "checksum256", "name": "block_id" }
            ]
        },
        {
            "name": "transaction_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "time_point_sec", "name": " expiration" },
                { "type": "checksum256", "name": "trx_id" }
            ]
        },
        {
            "name": "generated_transaction_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "checksum256", "name": "trx_id" },
                { "type": "name", "name": "sender" },
                { "type": "uint128", "name": "sender_id" },
                { "type": "name", "name": "payer" },
                { "type": "time_point", "name": " delay_until" },
                { "type": "time_point", "name": " expiration" },
                { "type": "time_point", "name": " published" },
                { "type": "bytes", "name": "packed_trx" }
            ]
        },
        {
            "name": "key_weight_v0", "fields": [
                { "type": "public_key", "name": "key" },
                { "type": "uint16", "name": "weight" }
            ]
        },
        {
            "name": "permission_level_v0", "fields": [
                { "type": "name", "name": "actor" },
                { "type": "name", "name": "permission" }
            ]
        },
        {
            "name": "permission_level_weight_v0", "fields": [
                { "type": "permission_level", "name": "permission" },
                { "type": "uint16", "name": "weight" }
            ]
        },
        {
            "name": "wait_weight_v0", "fields": [
                { "type": "uint32", "name": "wait_sec" },
                { "type": "uint16", "name": "weight" }
            ]
        },
        {
            "name": "shared_authority_v0", "fields": [
                { "type": "uint32", "name": "threshold" },
                { "type": "key_weight[]", "name": "keys" },
                { "type": "permission_level_weight[]", "name": "accounts" },
                { "type": "wait_weight[]", "name": "waits" }
            ]
        },
        {
            "name": "permission_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "uint64", "name": "usage_id" },
                { "type": "uint64", "name": "parent" },
                { "type": "name", "name": "owner" },
                { "type": "name", "name": "name" },
                { "type": "time_point", "name": " last_updated" },
                { "type": "shared_authority", "name": "auth" }
            ]
        },
        {
            "name": "permission_usage_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "time_point", "name": " last_used" }
            ]
        },
        {
            "name": "permission_link_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "name", "name": "account" },
                { "type": "name", "name": "code" },
                { "type": "name", "name": "message_type" },
                { "type": "name", "name": "required_permission" }
            ]
        },
        {
            "name": "resource_limits_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "name", "name": "owner" },
                { "type": "bool", "name": "pending" },
                { "type": "int64", "name": "net_weight" },
                { "type": "int64", "name": "cpu_weight" },
                { "type": "int64", "name": "ram_bytes" }
            ]
        },
        {
            "name": "usage_accumulator_v0", "fields": [
                { "type": "uint32", "name": "last_ordinal" },
                { "type": "uint64", "name": "value_ex" },
                { "type": "uint64", "name": "consumed" }
            ]
        },
        {
            "name": "resource_usage_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "name", "name": "owner" },
                { "type": "usage_accumulator", "name": "net_usage" },
                { "type": "usage_accumulator", "name": "cpu_usage" },
                { "type": "uint64", "name": "ram_usage" }
            ]
        },
        {
            "name": "resource_limits_state_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "usage_accumulator", "name": "average_block_net_usage" },
                { "type": "usage_accumulator", "name": "average_block_cpu_usage" },
                { "type": "uint64", "name": "pending_net_usage" },
                { "type": "uint64", "name": "pending_cpu_usage" },
                { "type": "uint64", "name": "total_net_weight" },
                { "type": "uint64", "name": "total_cpu_weight" },
                { "type": "uint64", "name": "total_ram_bytes" },
                { "type": "uint64", "name": "virtual_net_limit" },
                { "type": "uint64", "name": "virtual_cpu_limit" }
            ]
        },
        {
            "name": "resource_limits_ratio_v0", "fields": [
                { "type": "uint64", "name": "numerator" },
                { "type": "uint64", "name": "denominator" }
            ]
        },
        {
            "name": "elastic_limit_parameters_v0", "fields": [
                { "type": "uint64", "name": "target" },
                { "type": "uint64", "name": "max" },
                { "type": "uint32", "name": "periods" },
                { "type": "uint32", "name": "max_multiplier" },
                { "type": "resource_limits_ratio", "name": "contract_rate" },
                { "type": "resource_limits_ratio", "name": "expand_rate" }
            ]
        },
        {
            "name": "resource_limits_config_v0", "fields": [
                { "type": "uint64", "name": "id" },
                { "type": "elastic_limit_parameters", "name": "cpu_limit_parameters" },
                { "type": "elastic_limit_parameters", "name": "net_limit_parameters" },
                { "type": "uint32", "name": "account_cpu_usage_average_window" },
                { "type": "uint32", "name": "account_net_usage_average_window" }
            ]
        }
    ],
    "variants": [
        { "name": "request", "types": ["get_state_request_v0", "get_block_request_v0"] },
        { "name": "result", "types": ["get_state_result_v0", "get_block_result_v0"] },

        { "name": "table_delta", "types": ["table_delta_v0"] },
        { "name": "account", "types": ["account_v0"] },
        { "name": "account_sequence", "types": ["account_sequence_v0"] },
        { "name": "table_id", "types": ["table_id_v0"] },
        { "name": "key_value", "types": ["key_value_v0"] },
        { "name": "index64", "types": ["index64_v0"] },
        { "name": "index128", "types": ["index128_v0"] },
        { "name": "index256", "types": ["index256_v0"] },
        { "name": "index_double", "types": ["index_double_v0"] },
        { "name": "index_long_double", "types": ["index_long_double_v0"] },
        { "name": "producer_key", "types": ["producer_key_v0"] },
        { "name": "shared_producer_schedule_type", "types": ["shared_producer_schedule_type_v0"] },
        { "name": "chain_config", "types": ["chain_config_v0"] },
        { "name": "global_property", "types": ["global_property_v0"] },
        { "name": "dynamic_global_property", "types": ["dynamic_global_property_v0"] },
        { "name": "block_summary", "types": ["block_summary_v0"] },
        { "name": "transaction", "types": ["transaction_v0"] },
        { "name": "generated_transaction", "types": ["generated_transaction_v0"] },
        { "name": "key_weight", "types": ["key_weight_v0"] },
        { "name": "permission_level", "types": ["permission_level_v0"] },
        { "name": "permission_level_weight", "types": ["permission_level_weight_v0"] },
        { "name": "wait_weight", "types": ["wait_weight_v0"] },
        { "name": "shared_authority", "types": ["shared_authority_v0"] },
        { "name": "permission", "types": ["permission_v0"] },
        { "name": "permission_usage", "types": ["permission_usage_v0"] },
        { "name": "permission_link", "types": ["permission_link_v0"] },
        { "name": "resource_limits", "types": ["resource_limits_v0"] },
        { "name": "usage_accumulator", "types": ["usage_accumulator_v0"] },
        { "name": "resource_usage", "types": ["resource_usage_v0"] },
        { "name": "resource_limits_state", "types": ["resource_limits_state_v0"] },
        { "name": "resource_limits_ratio", "types": ["resource_limits_ratio_v0"] },
        { "name": "elastic_limit_parameters", "types": ["elastic_limit_parameters_v0"] },
        { "name": "resource_limits_config", "types": ["resource_limits_config_v0"] }
    ],
    "tables": [
        { "name": "account", "type": "account" },
        { "name": "account_sequence", "type": "account_sequence" },
        { "name": "table_id", "type": "table_id" },
        { "name": "key_value", "type": "key_value" },
        { "name": "index64", "type": "index64" },
        { "name": "index128", "type": "index128" },
        { "name": "index256", "type": "index256" },
        { "name": "index_double", "type": "index_double" },
        { "name": "index_long_double", "type": "index_long_double" },
        { "name": "global_property", "type": "global_property" },
        { "name": "dynamic_global_property", "type": "dynamic_global_property" },
        { "name": "block_summary", "type": "block_summary" },
        { "name": "transaction", "type": "transaction" },
        { "name": "generated_transaction", "type": "generated_transaction" },
        { "name": "permission", "type": "permission" },
        { "name": "permission_usage", "type": "permission_usage" },
        { "name": "permission_link", "type": "permission_link" },
        { "name": "resource_limits", "type": "resource_limits" },
        { "name": "resource_usage", "type": "resource_usage" },
        { "name": "resource_limits_state", "type": "resource_limits_state" },
        { "name": "resource_limits_config", "type": "resource_limits_config" }
    ]
})";
