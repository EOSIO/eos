#pragma once

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/cache_item.hpp>
#include <cyberway/chaindb/account_abi_info.hpp>

namespace cyberway { namespace chaindb {

    struct table_info {
        const account_name_t code     = 0;
        const scope_name_t   scope    = 0;
        const table_def*     table    = nullptr;
        const order_def*     pk_order = nullptr;

        account_abi_info     account_abi;

        table_info() = default;

        table_info(account_name_t c, scope_name_t s)
        : code(c), scope(s) {
        }

        bool is_valid() const {
            return nullptr != table;
        }

        table_name_t table_name() const {
            assert(is_valid());
            return table->name.value;
        }

        const abi_info& abi() const {
            assert(account_abi.has_abi_info());
            return account_abi.abi();
        }

        service_state to_service(const primary_key_t pk = primary_key::Unset) const {
            service_state service;

            service.code  = code;
            service.scope = scope;
            service.table = table_name();
            service.pk    = pk;

            return service;
        }
    }; // struct table_info

    struct index_info: public table_info {
        const index_def* index = nullptr;

        using table_info::table_info;

        index_info(const table_info& src)
        : table_info(src) {
        }

        bool is_valid() const {
            return table_info::is_valid() && nullptr != index;
        }

        index_name_t index_name() const {
            assert(is_valid());
            return index->name.value;
        }
    }; // struct index_info

} } // namespace cyberway::chaindb