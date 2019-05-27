#pragma once

#include <cyberway/chaindb/cache_item.hpp>
#include <cyberway/chaindb/abi_info.hpp>

#include <eosio/chain/account_object.hpp>

namespace cyberway { namespace chaindb {

    using account_object = eosio::chain::account_object;

    struct account_abi_info final {
        account_abi_info() = default;
        account_abi_info(cache_object_ptr);

        static const account_abi_info& system_abi_info();

        bool has_abi_info() const {
            return !!account_ptr_;
        }

        const abi_info& abi() const {
            return account().get_abi_info();
        }

    private:
        cache_object_ptr account_ptr_;

        using account_item_data = multi_index_item_data<account_object>;

        const account_object& account() const {
            assert(has_abi_info());
            return account_item_data::get_T(account_ptr_);
        }
    }; // struct account_abi_info

} } // namespace cyberway::chaindb