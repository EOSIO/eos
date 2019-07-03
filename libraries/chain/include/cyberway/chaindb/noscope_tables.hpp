#pragma once

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>

#include <cyberway/chain/domain_object.hpp>

namespace cyberway { namespace chaindb {

    inline bool is_noscope_table(const table_info& info) {
        if (!is_system_code(info.code)) {
            return false;
        }

        switch (info.table_name()) {
            case tag<eosio::chain::account_object>::get_code():
            case tag<eosio::chain::account_sequence_object>::get_code():
            case tag<eosio::chain::global_property_object>::get_code():
            case tag<eosio::chain::dynamic_global_property_object>::get_code():
            case tag<eosio::chain::block_summary_object>::get_code():
            case tag<eosio::chain::transaction_object>::get_code():
            case tag<eosio::chain::generated_transaction_object>::get_code():
            case tag<eosio::chain::permission_object>::get_code():
            case tag<eosio::chain::permission_usage_object>::get_code():
            case tag<eosio::chain::permission_link_object>::get_code():
            case tag<eosio::chain::resource_limits::resource_usage_object>::get_code():
            case tag<eosio::chain::resource_limits::resource_limits_config_object>::get_code():
            case tag<eosio::chain::resource_limits::resource_limits_state_object>::get_code():
            case tag<cyberway::chain::domain_object>::get_code():
            case tag<cyberway::chain::username_object>::get_code():
            case N(undo):
                return true;
        }
        return false;
    }

}  } // namespace cyberway::chaindb