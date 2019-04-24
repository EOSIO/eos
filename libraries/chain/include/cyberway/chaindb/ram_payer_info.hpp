#pragma once

#include <cyberway/chaindb/common.hpp>

namespace eosio { namespace chain {
    class apply_context;
    class transaction_context;
namespace resource_limits {
    class resource_limits_manager;
}
} } // namespace eosio::chain

namespace cyberway { namespace chaindb {

    using apply_context       = eosio::chain::apply_context;
    using resource_manager    = eosio::chain::resource_limits::resource_limits_manager;
    using transaction_context = eosio::chain::transaction_context;

    struct ram_payer_info final {
        apply_context*       apply_ctx       = nullptr;
        resource_manager*    resource_mng    = nullptr;
        transaction_context* transaction_ctx = nullptr;

        const account_name   owner;
        const account_name   payer;
        size_t               precalc_size = 0;
        bool                 in_ram = true;

        ram_payer_info() = default;

        ram_payer_info(apply_context& c, account_name o, account_name p, const size_t s = 0)
        : apply_ctx(&c), owner(std::move(o)), payer(std::move(p)), precalc_size(s) {
        }
        
        ram_payer_info(resource_manager& r, account_name o)
        : resource_mng(&r), owner(std::move(o)) {
        }

        ram_payer_info(transaction_context& t, account_name o, account_name p, const size_t s = 0)
        : transaction_ctx(&t), owner(std::move(o)), payer(std::move(p)), precalc_size(s) {
        }

        void add_usage(const account_name, const int64_t delta) const;
    }; // struct ram_payer_info

} } // namespace cyberway::chaindb
