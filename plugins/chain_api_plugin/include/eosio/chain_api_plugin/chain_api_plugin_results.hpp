#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/asset.hpp>

#include <fc/optional.hpp>

namespace eosio {

    struct permission {
        chain::name      perm_name;
        chain::name      parent;
        chain::authority required_auth;
    };

    struct account_resource_limit {
       int64_t used = 0; ///< quantity used in current window
       int64_t available = 0; ///< quantity available in current window (based upon fractional reserve)
       int64_t max = 0; ///< max per window under current congestion
    };

    struct get_account_results {
       chain::name                account_name;
       uint32_t                   head_block_num = 0;
       fc::time_point             head_block_time;
       bool                       privileged = false;
       fc::time_point             last_code_update;
       fc::time_point             created;
       fc::optional<chain::asset> core_liquid_balance;

       int64_t                    ram_quota  = 0;
       int64_t                    net_weight = 0;
       int64_t                    cpu_weight = 0;

       account_resource_limit     net_limit;
       account_resource_limit     cpu_limit;
       account_resource_limit     ram_limit;
       account_resource_limit     storage_limit;

       int64_t                    ram_usage = 0;

       std::vector<permission>    permissions;
       fc::variant                total_resources;
       fc::variant                self_delegated_bandwidth;
       fc::variant                refund_request;
       fc::variant                voter_info;
    };

    struct get_code_results {
       chain::name                  account_name;
       std::string                  wast;
       std::string                  wasm;
       fc::sha256                   code_hash;
       fc::optional<chain::abi_def> abi;
    };

    struct get_code_hash_results {
       chain::name account_name;
       fc::sha256  code_hash;
    };

    struct get_abi_results {
       chain::name                  account_name;
       fc::optional<chain::abi_def> abi;
    };

    struct get_raw_code_and_abi_results {
       chain::name account_name;
       std::string wasm;
       std::string abi;
    };

    struct get_raw_abi_results {
       chain::name               account_name;
       fc::sha256                code_hash;
       fc::sha256                abi_hash;
       fc::optional<std::string> abi;
    };

    struct resolve_names_item {
        fc::optional<chain::name> resolved_domain;
        fc::optional<chain::name> resolved_username;
    };

    using resolve_names_results = std::vector<resolve_names_item>;

    struct abi_json_to_bin_result {
       std::vector<char> binargs;
    };

    struct abi_bin_to_json_result {
       fc::variant args;
    };

    struct get_required_keys_result {
       chain::flat_set<chain::public_key_type> required_keys;
    };

    using get_transaction_id_result = chain::transaction_id_type;

    struct get_table_rows_result {
        std::vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object
        bool                     more = false; ///< true if last element in data is not the end and sizeof data() < limit
    };

    struct get_table_by_scope_result_row {
        chain::name code;
        chain::name scope;
        chain::name table;
        chain::name payer;
        uint32_t    count;
    };
    struct get_table_by_scope_result {
        std::vector<get_table_by_scope_result_row> rows;
        std::string                                more; ///< fill lower_bound with this value to fetch more rows
    };

    struct get_currency_stats_result {
        chain::asset        supply;
        chain::asset        max_supply;
        chain::account_name issuer;
    };

    struct get_producers_result {
        std::vector<fc::variant> rows; ///< one row per item, either encoded as hex string or JSON object
        uint64_t                  total_producer_vote_weight;
        std::string              more; ///< fill lower_bound with this value to fetch more rows
    };

    struct get_producer_schedule_result {
        fc::variant active;
        fc::variant pending;
        fc::variant proposed;
    };

    struct get_scheduled_transactions_result {
        fc::variants transactions;
        std::string  more; ///< fill lower_bound with this to fetch next set of transactions
    };

    struct get_proxy_status_result {
        chain::account_name account;
        chain::symbol symbol;
        uint8_t proxylevel;
        uint32_t proxies_count;
    };
}

FC_REFLECT( eosio::get_table_rows_result, (rows)(more) )
FC_REFLECT( eosio::get_table_by_scope_result_row, (code)(scope)(table)(payer)(count))
FC_REFLECT( eosio::get_table_by_scope_result, (rows)(more) )
FC_REFLECT( eosio::get_currency_stats_result, (supply)(max_supply)(issuer))
FC_REFLECT( eosio::get_producers_result, (rows)(total_producer_vote_weight)(more) )
FC_REFLECT( eosio::get_producer_schedule_result, (active)(pending)(proposed) )
FC_REFLECT( eosio::get_scheduled_transactions_result, (transactions)(more) )
FC_REFLECT( eosio::permission, (perm_name)(parent)(required_auth) )
FC_REFLECT( eosio::account_resource_limit, (used)(available)(max) )
FC_REFLECT( eosio::get_account_results,
            (account_name)(head_block_num)(head_block_time)(privileged)(last_code_update)(created)
            (core_liquid_balance)(ram_quota)(net_weight)(cpu_weight)(net_limit)(cpu_limit)(ram_limit)(storage_limit)(ram_usage)(permissions)
            (total_resources)(self_delegated_bandwidth)(refund_request)(voter_info) )
FC_REFLECT( eosio::get_code_results, (account_name)(code_hash)(wast)(wasm)(abi) )
FC_REFLECT( eosio::get_code_hash_results, (account_name)(code_hash) )
FC_REFLECT( eosio::get_abi_results, (account_name)(abi) )
FC_REFLECT( eosio::get_raw_code_and_abi_results, (account_name)(wasm)(abi) )
FC_REFLECT( eosio::get_raw_abi_results, (account_name)(code_hash)(abi_hash)(abi) )
FC_REFLECT( eosio::abi_json_to_bin_result, (binargs) )
FC_REFLECT( eosio::abi_bin_to_json_result, (args) )
FC_REFLECT( eosio::get_required_keys_result, (required_keys) )
FC_REFLECT(eosio::resolve_names_item, (resolved_domain)(resolved_username))
FC_REFLECT( eosio::get_proxy_status_result, (account)(symbol)(proxylevel)(proxies_count))
