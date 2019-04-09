#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/asset.hpp>

#include <fc/optional.hpp>

namespace eosio {

    struct get_info_results {
        std::string             server_version;
        chain::chain_id_type    chain_id;
        uint32_t                head_block_num = 0;
        uint32_t                last_irreversible_block_num = 0;
        chain::block_id_type    last_irreversible_block_id;
        chain::block_id_type    head_block_id;
        fc::time_point          head_block_time;
        chain::account_name            head_block_producer;

        uint64_t                virtual_block_cpu_limit = 0;
        uint64_t                virtual_block_net_limit = 0;

        uint64_t                block_cpu_limit = 0;
        uint64_t                block_net_limit = 0;
        fc::optional<std::string>        server_version_string;
    };

    struct push_block_results{};

    struct push_transaction_results {
        chain::transaction_id_type  transaction_id;
        fc::variant                 processed;
    };

    using push_transactions_results = std::vector<push_transaction_results>;
}

FC_REFLECT(eosio::get_info_results,
(server_version)(chain_id)(head_block_num)(last_irreversible_block_num)(last_irreversible_block_id)(head_block_id)(head_block_time)(head_block_producer)(virtual_block_cpu_limit)(virtual_block_net_limit)(block_cpu_limit)(block_net_limit)(server_version_string) )

FC_REFLECT_EMPTY( eosio::push_block_results)

FC_REFLECT( eosio::push_transaction_results, (transaction_id)(processed) )


