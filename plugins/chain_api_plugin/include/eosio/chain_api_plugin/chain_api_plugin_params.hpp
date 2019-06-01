#pragma once

#include <boost/container/flat_set.hpp>

#include <eosio/chain/types.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/symbol.hpp>
#include <eosio/chain/transaction.hpp>

#include <fc/optional.hpp>

namespace eosio {

    struct get_account_params {
       chain::name                 account_name;
       fc::optional<chain::symbol> expected_core_symbol;
    };

    struct get_code_params {
       chain::name account_name;
       bool code_as_wasm = false;
    };

    struct get_code_hash_params {
       chain::name account_name;
    };

    struct get_abi_params {
       chain::name account_name;
    };

    struct get_raw_code_and_abi_params {
       chain::name account_name;
    };

    struct get_raw_abi_params {
        chain::name                account_name;
        fc::optional<fc::sha256>   abi_hash;
     };

    using resolve_names_params = std::vector<std::string>;

    struct abi_json_to_bin_params {
       chain::name code;
       chain::name action;
       fc::variant args;
    };

    struct abi_bin_to_json_params {
       chain::name       code;
       chain::name       action;
       std::vector<char> binargs;
    };

    struct get_required_keys_params {
       fc::variant                                        transaction;
       boost::container::flat_set<chain::public_key_type> available_keys;
    };

    using get_transaction_id_params = chain::transaction;

    struct get_table_rows_params {
        bool                json = false;
        chain::name         code;
        std::string         scope;
        chain::name         table;
        std::string         table_key;
        fc::variant         lower_bound;
        fc::variant         upper_bound;
        uint32_t            limit = 10;
        chain::name         index;
        std::string         encode_type{"dec"}; //dec, hex , default=dec
        fc::optional<bool>  reverse;
        fc::optional<bool>  show_payer; // show RAM pyer
    };

    struct get_table_by_scope_params {
        chain::name         code; // mandatory
        chain::name         table = 0; // optional, act as filter
        std::string         lower_bound; // lower bound of scope, optional
        std::string         upper_bound; // upper bound of scope, optional
        uint32_t            limit = 10;
        fc::optional<bool>  reverse;
    };

    struct get_currency_balance_params {
        chain::name                 code;
        chain::name                 account;
        fc::optional<std::string>   symbol;
    };

    struct get_currency_stats_params {
        chain::name code;
        std::string symbol;
    };

    struct get_producers_params {
        bool        json = false;
        std::string lower_bound;
        uint32_t    limit = 50;
    };

    struct get_agent_public_key_params {
        chain::account_name account;
        chain::symbol symbol;
    };

    struct get_producer_schedule_params {
    };

    struct get_proxy_status_params {
        chain::account_name account;
        chain::symbol symbol;
    };

    struct get_proxylevel_limits_params {
        chain::symbol symbol;
    };

    struct get_scheduled_transactions_params {
        bool        json = false;
        std::string lower_bound;  /// timestamp OR transaction ID
        uint32_t    limit = 50;
    };

}

FC_REFLECT( eosio::get_table_rows_params, (json)(code)(scope)(table)(table_key)(lower_bound)(upper_bound)(limit)(index)(encode_type)(reverse)(show_payer) )
FC_REFLECT( eosio::get_table_by_scope_params, (code)(table)(lower_bound)(upper_bound)(limit)(reverse) )
FC_REFLECT( eosio::get_currency_balance_params, (code)(account)(symbol))
FC_REFLECT( eosio::get_currency_stats_params, (code)(symbol))
FC_REFLECT( eosio::get_producers_params, (json)(lower_bound)(limit) )
FC_REFLECT_EMPTY( eosio::get_producer_schedule_params )
FC_REFLECT( eosio::get_scheduled_transactions_params, (json)(lower_bound)(limit) )
FC_REFLECT( eosio::get_account_params, (account_name)(expected_core_symbol) )
FC_REFLECT( eosio::get_code_params, (account_name)(code_as_wasm) )
FC_REFLECT( eosio::get_code_hash_params, (account_name) )
FC_REFLECT( eosio::get_abi_params, (account_name) )
FC_REFLECT( eosio::get_raw_code_and_abi_params, (account_name) )
FC_REFLECT( eosio::get_raw_abi_params, (account_name)(abi_hash) )
FC_REFLECT( eosio::abi_json_to_bin_params, (code)(action)(args) )
FC_REFLECT( eosio::abi_bin_to_json_params, (code)(action)(binargs) )
FC_REFLECT( eosio::get_required_keys_params, (transaction)(available_keys) )
FC_REFLECT( eosio::get_agent_public_key_params, (account)(symbol))
FC_REFLECT( eosio::get_proxy_status_params, (account)(symbol))
FC_REFLECT( eosio::get_proxylevel_limits_params, (symbol))
