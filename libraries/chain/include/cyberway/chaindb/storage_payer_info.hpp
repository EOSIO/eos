#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {
    class apply_context;
    class transaction_context;
namespace resource_limits {
    class resource_limits_manager;
}
} } // namespace eosio::chain

namespace cyberway { namespace chaindb {

    using account_name        = eosio::chain::account_name;
    using apply_context       = eosio::chain::apply_context;
    using resource_manager    = eosio::chain::resource_limits::resource_limits_manager;
    using transaction_context = eosio::chain::transaction_context;

    struct table_info;
    struct object_value;

    struct storage_payer_info final {
        apply_context*       apply_ctx       = nullptr;
        resource_manager*    resource_mng    = nullptr;
        transaction_context* transaction_ctx = nullptr;

        account_name owner;
        account_name payer;
        int          size   = 0;
        int          delta  = 0;
        bool         in_ram = true;

        storage_payer_info() = default;

        storage_payer_info(apply_context& c, account_name o, account_name p)
        : apply_ctx(&c), owner(std::move(o)), payer(std::move(p)) {
        }

        storage_payer_info(resource_manager& r, account_name o, account_name p)
        : resource_mng(&r), owner(std::move(o)), payer(std::move(p)) {
        }

        storage_payer_info(transaction_context& t, account_name o, account_name p)
        : transaction_ctx(&t), owner(std::move(o)), payer(std::move(p)) {
        }

        void calc_usage(const table_info&, const object_value&);
        void add_usage();

        void get_payer_from(const object_value&);
        void set_payer_in(object_value&) const;
    }; // struct storage_payer_info

} } // namespace cyberway::chaindb

FC_REFLECT(cyberway::chaindb::storage_payer_info, (owner)(payer)(size)(delta)(in_ram))
