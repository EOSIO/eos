#include <cyberway/chaindb/account_abi_info.hpp>
#include <cyberway/chaindb/undo_state.hpp>

namespace eosio { namespace chain {
    namespace chaindb = cyberway::chaindb;

    void account_object::generate_abi_info() {
        if (!abi_info_) {
            abi_info_ = std::make_unique<chaindb::abi_info>(name, get_abi());
        }
    }

    const chaindb::abi_info& account_object::get_abi_info() const {
        return *abi_info_.get();
    }

} } // namespace eosio::chain

namespace cyberway { namespace  chaindb {

    namespace config = eosio::chain::config;

    account_abi_info::account_abi_info(cache_object_ptr account_ptr)
    : account_ptr_(std::move(account_ptr)) {
        if (!account_ptr_) {
            return;
        }

        assert(account_ptr_->service().table == chaindb::tag<account_object>::get_code());

        auto& a = account();
        if (a.abi.empty()) {
            account_ptr_.reset();
            return;
        }

        const_cast<account_object&>(a).generate_abi_info();
    }

    const account_abi_info& account_abi_info::system_abi_info() {
        static account_abi_info system_abi_info([&]() {
            service_state service;

            service.table = chaindb::tag<account_object>::get_code();
            service.pk    = config::system_account_name;

            auto obj_ptr  = std::make_unique<cache_object>(std::move(service));

            obj_ptr->set_data<account_item_data>(config::system_account_name, [&](auto& a) {
                auto def = eosio::chain::eosio_contract_abi();
                undo_stack::add_abi_tables(def);
                a.set_abi(def);
                a.generate_abi_info();
            });

            return cache_object_ptr(obj_ptr.release());
        }());

        return system_abi_info;
    }

} } // namespace cyberway::chaindb