#pragma once

#include <cyberway/chaindb/cache_item.hpp>
#include <cyberway/chaindb/abi_info.hpp>

#include <eosio/chain/account_object.hpp>

namespace cyberway { namespace chaindb {

    using account_object = eosio::chain::account_object;

    struct account_abi_info final {
        account_abi_info() = default;
        account_abi_info(cache_object_ptr);

        bool has_abi_info() const {
            return !!account_ptr_;
        }

        const abi_info& abi() const {
            return account().get_abi_info();
        }

    private:
        friend struct system_abi_info_impl;
        cache_object_ptr account_ptr_;

        const account_object& account() const {
            assert(has_abi_info());
            return multi_index_item_data<account_object>::get_T(account_ptr_);
        }
    }; // struct account_abi_info

    struct system_abi_info final {
        system_abi_info() = delete;
        system_abi_info(const driver_interface&);
        ~system_abi_info();

        void init_abi() const;
        void set_abi(blob) const;

        const abi_info& abi() const {
            return info_.abi();
        }

        const index_info& account_index() const {
            return index_;
        }

        account_abi_info info() const {
            return info_;
        }

    private:
        std::unique_ptr<system_abi_info_impl> impl_;
        const account_abi_info& info_;
        const index_info& index_;
    }; // struct system_abi_info

} } // namespace cyberway::chaindb
