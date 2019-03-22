#pragma once

#include <cyberway/chaindb/common.hpp>

namespace eosio { namespace chain {
        class apply_context;
namespace resource_limits {
        class resource_limits_manager;
}
} } // namespace eosio::chain

namespace cyberway { namespace chaindb {

    using eosio::chain::apply_context;
    using eosio::chain::resource_limits::resource_limits_manager;

    struct ram_payer_info final {
        apply_context* ctx   = nullptr; // maybe unavailable - see eosio controller
        account_name   payer;
        size_t         precalc_size = 0;
        resource_limits_manager* rl = nullptr;

        ram_payer_info() = default;

        ram_payer_info(apply_context& c, account_name p = account_name(), const size_t s = 0)
            : ctx(&c), payer(std::move(p)), precalc_size(s) {
        }
        
        ram_payer_info(resource_limits_manager& r, account_name p = account_name(), const size_t s = 0)
            : ctx(nullptr), payer(std::move(p)), precalc_size(s), rl(&r) {
        }

        void add_usage(const account_name, const int64_t delta) const;
    }; // struct ram_payer_info

} } // namespace cyberway::chaindb
