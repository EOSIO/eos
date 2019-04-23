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
        const size_t         precalc_size = 0;

        ram_payer_info() = default;

        ram_payer_info(apply_context& c, account_name p = {}, account_name o = {}, const size_t s = 0)
        : apply_ctx(&c), owner(select_owner(p, o)), payer(std::move(p)), precalc_size(s) {
        }
        
        ram_payer_info(resource_manager& r, account_name p = {}, account_name o = {}, const size_t s = 0)
        : resource_mng(&r), owner(select_owner(p, o)), payer(std::move(p)), precalc_size(s) {
        }

        ram_payer_info(transaction_context& t, account_name p = {}, account_name o = {}, const size_t s = 0)
        : transaction_ctx(&t), owner(select_owner(p, o)), payer(std::move(p)), precalc_size(s) {
        }

        void add_usage(const account_name, const int64_t delta) const;

    private:
        static account_name select_owner(account_name p, account_name o) {
            if (o.empty()) {
                return p;
            } else {
                return o;
            }
        }
    }; // struct ram_payer_info

} } // namespace cyberway::chaindb
