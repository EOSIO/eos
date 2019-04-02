extern const char* const state_history_plugin_abi = R"({
    "version": "eosio::abi/1.1",
    "structs": [
        {
            "name": "get_status_request_v0", "fields": []
        },
        {
            "name": "block_position", "fields": [
                { "name": "block_num", "type": "uint32" },
                { "name": "block_id", "type": "checksum256" }
            ]
        },
        {
            "name": "get_status_result_v0", "fields": [
                { "name": "head", "type": "block_position" },
                { "name": "last_irreversible", "type": "block_position" },
                { "name": "trace_begin_block", "type": "uint32" },
                { "name": "trace_end_block", "type": "uint32" },
                { "name": "chain_state_begin_block", "type": "uint32" },
                { "name": "chain_state_end_block", "type": "uint32" }
            ]
        },
        {
            "name": "get_blocks_request_v0", "fields": [
                { "name": "start_block_num", "type": "uint32" },
                { "name": "end_block_num", "type": "uint32" },
                { "name": "max_messages_in_flight", "type": "uint32" },
                { "name": "have_positions", "type": "block_position[]" },
                { "name": "irreversible_only", "type": "bool" },
                { "name": "fetch_block", "type": "bool" },
                { "name": "fetch_traces", "type": "bool" },
                { "name": "fetch_deltas", "type": "bool" }
            ]
        },
        {
            "name": "get_blocks_ack_request_v0", "fields": [
                { "name": "num_messages", "type": "uint32" }
            ]
        },
        {
            "name": "get_blocks_result_v0", "fields": [
                { "name": "head", "type": "block_position" },
                { "name": "last_irreversible", "type": "block_position" },
                { "name": "this_block", "type": "block_position?" },
                { "name": "prev_block", "type": "block_position?" },
                { "name": "block", "type": "bytes?" },
                { "name": "traces", "type": "bytes?" },
                { "name": "deltas", "type": "bytes?" }
            ]
        },
        {
            "name": "row", "fields": [
                { "name": "present", "type": "bool" },
                { "name": "data", "type": "bytes" }
            ]
        },
        {
            "name": "table_delta_v0", "fields": [
                { "name": "name", "type": "string" },
                { "name": "rows", "type": "row[]" }
            ]
        },
        {
            "name": "action", "fields": [
                { "name": "account", "type": "name" },
                { "name": "name", "type": "name" },
                { "name": "authorization", "type": "permission_level[]" },
                { "name": "data", "type": "bytes" }
            ]
        },
        {
            "name": "account_auth_sequence", "fields": [
                { "name": "account", "type": "name" },
                { "name": "sequence", "type": "uint64" }
            ]
        },
        {
            "name": "action_receipt_v0", "fields": [
                { "name": "receiver", "type": "name" },
                { "name": "act_digest", "type": "checksum256" },
                { "name": "global_sequence", "type": "uint64" },
                { "name": "recv_sequence", "type": "uint64" },
                { "name": "auth_sequence", "type": "account_auth_sequence[]" },
                { "name": "code_sequence", "type": "varuint32" },
                { "name": "abi_sequence", "type": "varuint32" }
            ]
        },
        {
            "name": "account_delta", "fields": [
                { "name": "account", "type": "name" },
                { "name": "delta", "type": "int64" }
            ]
        },
        {
            "name": "action_trace_v0", "fields": [
                { "name": "action_ordinal", "type": "varuint32" },
                { "name": "creator_action_ordinal", "type": "varuint32" },
                { "name": "parent_action_ordinal", "type": "varuint32" },
                { "name": "receipt", "type": "action_receipt?" },
                { "name": "receiver", "type": "name" },
                { "name": "act", "type": "action" },
                { "name": "context_free", "type": "bool" },
                { "name": "elapsed", "type": "int64" },
                { "name": "console", "type": "string" },
                { "name": "account_ram_deltas", "type": "account_delta[]" },
                { "name": "except", "type": "string?" },
            ]
        },
        {
            "name": "transaction_trace_v0", "fields": [
                { "name": "id", "type": "checksum256" },
                { "name": "status", "type": "uint8" },
                { "name": "cpu_usage_us", "type": "uint32" },
                { "name": "net_usage_words", "type": "varuint32" },
                { "name": "elapsed", "type": "int64" },
                { "name": "net_usage", "type": "uint64" },
                { "name": "scheduled", "type": "bool" },
                { "name": "action_traces", "type": "action_trace[]" },
                { "name": "account_ram_delta", "type": "account_delta?" },
                { "name": "except", "type": "string?" },
                { "name": "failed_dtrx_trace", "type": "transaction_trace?" }
            ]
        },
        {
            "name": "packed_transaction", "fields": [
                { "name": "signatures", "type": "signature[]" },
                { "name": "compression", "type": "uint8" },
                { "name": "packed_context_free_data", "type": "bytes" },
                { "name": "packed_trx", "type": "bytes" }
            ]
        },
        {
            "name": "transaction_receipt_header", "fields": [
                { "name": "status", "type": "uint8" },
                { "name": "cpu_usage_us", "type": "uint32" },
                { "name": "net_usage_words", "type": "varuint32" }
            ]
        },
        {
            "name": "transaction_receipt", "base": "transaction_receipt_header", "fields": [
                { "name": "trx", "type": "transaction_variant" }
            ]
        },
        {
            "name": "extension", "fields": [
                { "name": "type", "type": "uint16" },
                { "name": "data", "type": "bytes" }
            ]
        },
        {
            "name": "block_header", "fields": [
                { "name": "timestamp", "type": "block_timestamp_type" },
                { "name": "producer", "type": "name" },
                { "name": "confirmed", "type": "uint16" },
                { "name": "previous", "type": "checksum256" },
                { "name": "transaction_mroot", "type": "checksum256" },
                { "name": "action_mroot", "type": "checksum256" },
                { "name": "schedule_version", "type": "uint32" },
                { "name": "new_producers", "type": "producer_schedule?" },
                { "name": "header_extensions", "type": "extension[]" }
            ]
        },
        {
            "name": "signed_block_header", "base": "block_header", "fields": [
                { "name": "producer_signature", "type": "signature" }
            ]
        },
        {
            "name": "signed_block", "base": "signed_block_header", "fields": [
                { "name": "transactions", "type": "transaction_receipt[]" },
                { "name": "block_extensions", "type": "extension[]" }
            ]
        },
        {   "name": "transaction_header", "fields": [
                { "name": "expiration", "type": "time_point_sec" },
                { "name": "ref_block_num", "type": "uint16" },
                { "name": "ref_block_prefix", "type": "uint32" },
                { "name": "max_net_usage_words", "type": "varuint32" },
                { "name": "max_cpu_usage_ms", "type": "uint8" },
                { "name": "delay_sec", "type": "varuint32" }
            ]
        },
        {   "name": "transaction", "base": "transaction_header", "fields": [
                { "name": "context_free_actions", "type": "action[]" },
                { "name": "actions", "type": "action[]" },
                { "name": "transaction_extensions", "type": "extension[]" }
            ]
        },
        {
            "name": "account_v0", "fields": [
                { "type": "name", "name": "name" },
                { "type": "uint8", "name": "vm_type" },
                { "type": "uint8", "name": "vm_version" },
                { "type": "bool", "name": "privileged" },
                { "type": "time_point", "name": "last_code_update" },
                { "type": "checksum256", "name": "code_version" },
                { "type": "block_timestamp_type", "name": "creation_date" },
                { "type": "bytes", "name": "code" },
                { "type": "bytes", "name": "abi" }
            ]
        },
        {
            "name": "contract_table_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "name", "name": "payer" }
            ]
        },
        {
            "name": "contract_row_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "bytes", "name": "value" }
            ]
        },
        {
            "name": "contract_index64_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "uint64", "name": "secondary_key" }
            ]
        },
        {
            "name": "contract_index128_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "uint128", "name": "secondary_key" }
            ]
        },
        {
            "name": "contract_index256_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "checksum256", "name": "secondary_key" }
            ]
        },
        {
            "name": "contract_index_double_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "float64", "name": "secondary_key" }
            ]
        },
        {
            "name": "contract_index_long_double_v0", "fields": [
                { "type": "name", "name": "code" },
                { "type": "name", "name": "scope" },
                { "type": "name", "name": "table" },
                { "type": "uint64", "name": "primary_key" },
                { "type": "name", "name": "payer" },
                { "type": "float128", "name": "secondary_key" }
            ]
        },
        {
            "name": "producer_key", "fields": [
                { "type": "name", "name": "producer_name" },
                { "type": "public_key", "name": "block_signing_key" }
            ]
        },
        {
            "name": "producer_schedule", "fields": [
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
                { "type": "uint32?", "name": "proposed_schedule_block_num" },
                { "type": "producer_schedule", "name": "proposed_schedule" },
                { "type": "chain_config", "name": "configuration" }
            ]
        },
        {
            "name": "generated_transaction_v0", "fields": [
                { "type": "name", "name": "sender" },
                { "type": "uint128", "name": "sender_id" },
                { "type": "name", "name": "payer" },
                { "type": "checksum256", "name": "trx_id" },
                { "type": "bytes", "name": "packed_trx" }
            ]
        },
        {
            "name": "key_weight", "fields": [
                { "type": "public_key", "name": "key" },
                { "type": "uint16", "name": "weight" }
            ]
        },
        {
            "name": "permission_level", "fields": [
                { "type": "name", "name": "actor" },
                { "type": "name", "name": "permission" }
            ]
        },
        {
            "name": "permission_level_weight", "fields": [
                { "type": "permission_level", "name": "permission" },
                { "type": "uint16", "name": "weight" }
            ]
        },
        {
            "name": "wait_weight", "fields": [
                { "type": "uint32", "name": "wait_sec" },
                { "type": "uint16", "name": "weight" }
            ]
        },
        {
            "name": "authority", "fields": [
                { "type": "uint32", "name": "threshold" },
                { "type": "key_weight[]", "name": "keys" },
                { "type": "permission_level_weight[]", "name": "accounts" },
                { "type": "wait_weight[]", "name": "waits" }
            ]
        },
        {
            "name": "permission_v0", "fields": [
                { "type": "name", "name": "owner" },
                { "type": "name", "name": "name" },
                { "type": "name", "name": "parent" },
                { "type": "time_point", "name": "last_updated" },
                { "type": "authority", "name": "auth" }
            ]
        },
        {
            "name": "permission_link_v0", "fields": [
                { "type": "name", "name": "account" },
                { "type": "name", "name": "code" },
                { "type": "name", "name": "message_type" },
                { "type": "name", "name": "required_permission" }
            ]
        },
        {
            "name": "resource_limits_v0", "fields": [
                { "type": "name", "name": "owner" },
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
                { "type": "name", "name": "owner" },
                { "type": "usage_accumulator", "name": "net_usage" },
                { "type": "usage_accumulator", "name": "cpu_usage" },
                { "type": "uint64", "name": "ram_usage" }
            ]
        },
        {
            "name": "resource_limits_state_v0", "fields": [
                { "type": "usage_accumulator", "name": "average_block_net_usage" },
                { "type": "usage_accumulator", "name": "average_block_cpu_usage" },
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
                { "type": "elastic_limit_parameters", "name": "cpu_limit_parameters" },
                { "type": "elastic_limit_parameters", "name": "net_limit_parameters" },
                { "type": "uint32", "name": "account_cpu_usage_average_window" },
                { "type": "uint32", "name": "account_net_usage_average_window" }
            ]
        }
    ],
    "types": [
        { "new_type_name": "transaction_id", "type": "checksum256" }
    ],
    "variants": [
        { "name": "request", "types": ["get_status_request_v0", "get_blocks_request_v0", "get_blocks_ack_request_v0"] },
        { "name": "result", "types": ["get_status_result_v0", "get_blocks_result_v0"] },

        { "name": "action_receipt", "types": ["action_receipt_v0"] },
        { "name": "action_trace", "types": ["action_trace_v0"] },
        { "name": "transaction_trace", "types": ["transaction_trace_v0"] },
        { "name": "transaction_variant", "types": ["transaction_id", "packed_transaction"] },

        { "name": "table_delta", "types": ["table_delta_v0"] },
        { "name": "account", "types": ["account_v0"] },
        { "name": "contract_table", "types": ["contract_table_v0"] },
        { "name": "contract_row", "types": ["contract_row_v0"] },
        { "name": "contract_index64", "types": ["contract_index64_v0"] },
        { "name": "contract_index128", "types": ["contract_index128_v0"] },
        { "name": "contract_index256", "types": ["contract_index256_v0"] },
        { "name": "contract_index_double", "types": ["contract_index_double_v0"] },
        { "name": "contract_index_long_double", "types": ["contract_index_long_double_v0"] },
        { "name": "chain_config", "types": ["chain_config_v0"] },
        { "name": "global_property", "types": ["global_property_v0"] },
        { "name": "generated_transaction", "types": ["generated_transaction_v0"] },
        { "name": "permission", "types": ["permission_v0"] },
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
        { "name": "account", "type": "account", "key_names": ["name"] },
        { "name": "contract_table", "type": "contract_table", "key_names": ["code", "scope", "table"] },
        { "name": "contract_row", "type": "contract_row", "key_names": ["code", "scope", "table", "primary_key"] },
        { "name": "contract_index64", "type": "contract_index64", "key_names": ["code", "scope", "table", "primary_key"] },
        { "name": "contract_index128", "type": "contract_index128", "key_names": ["code", "scope", "table", "primary_key"] },
        { "name": "contract_index256", "type": "contract_index256", "key_names": ["code", "scope", "table", "primary_key"] },
        { "name": "contract_index_double", "type": "contract_index_double", "key_names": ["code", "scope", "table", "primary_key"] },
        { "name": "contract_index_long_double", "type": "contract_index_long_double", "key_names": ["code", "scope", "table", "primary_key"] },
        { "name": "global_property", "type": "global_property", "key_names": [] },
        { "name": "generated_transaction", "type": "generated_transaction", "key_names": ["sender", "sender_id"] },
        { "name": "permission", "type": "permission", "key_names": ["owner", "name"] },
        { "name": "permission_link", "type": "permission_link", "key_names": ["account", "code", "message_type"] },
        { "name": "resource_limits", "type": "resource_limits", "key_names": ["owner"] },
        { "name": "resource_usage", "type": "resource_usage", "key_names": ["owner"] },
        { "name": "resource_limits_state", "type": "resource_limits_state", "key_names": [] },
        { "name": "resource_limits_config", "type": "resource_limits_config", "key_names": [] }
    ]
})";
