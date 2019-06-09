#include <cyberway/chaindb/account_abi_info.hpp>
#include <cyberway/chaindb/table_info.hpp>
#include <cyberway/chaindb/driver_interface.hpp>

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

    account_abi_info::account_abi_info(cache_object_ptr account_ptr) {
        init(std::move(account_ptr));
    }

    account_abi_info::account_abi_info(account_name_t code, abi_def def) {
        init(code, std::move(def));
    }

    account_abi_info::account_abi_info(account_name_t code, blob b) {
        init(code, std::move(b.data));
    }

    template<typename Abi> void account_abi_info::init(account_name_t code, Abi&& def) {
        object_value obj;

        obj.service.table = chaindb::tag<account_object>::get_code();
        obj.service.pk    = code;

        auto cache_ptr  = std::make_unique<cache_object>(std::move(obj));
        cache_ptr->set_data<multi_index_item_data<account_object>>(code, [&](auto& a) {
            a.set_abi(std::forward<Abi>(def));
        });

        init(cache_ptr.release());
    }

    void account_abi_info::init(cache_object_ptr account_ptr) {
        account_ptr_ = std::move(account_ptr);
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

    struct system_abi_info_impl final {
        account_abi_info info;
        index_info       index;

        system_abi_info_impl(const driver_interface& driver)
        : driver_(driver) {
            init_info();
        }

        void init_info() {
            init_info(eosio::chain::eosio_contract_abi());

            auto obj = driver_.object_by_pk(index, config::system_account_name);
            if (obj.value.is_object()) {
                auto& abi = obj.value["abi"];
                if (abi.is_blob() && !abi.get_blob().data.empty()) {
                    init_info(abi.get_blob());
                }
            }
        }

        template <typename Abi> void init_info(Abi&& def) {
            info = account_abi_info(config::system_account_name, std::forward<Abi>(def));
            init_index();
        }

    private:
        const driver_interface& driver_;

        void init_index() {
            auto& abi   = info.abi();
            auto  table = abi.find_table(chaindb::tag<account_object>::get_code());
            index.table    = table;
            index.index    = abi.find_pk_index(*table);
            index.pk_order = abi.find_pk_order(*table);
        }
    }; // struct system_abi_info_impl

    system_abi_info::system_abi_info(const driver_interface& driver)
    : impl_(std::make_unique<system_abi_info_impl>(driver)),
      info_(impl_->info),
      index_(impl_->index) {
    }

    system_abi_info::~system_abi_info() = default;

    void system_abi_info::init_abi() const {
        impl_->init_info();
    }

    void system_abi_info::set_abi(abi_def def) const {
        impl_->init_info(std::move(def));
    }

} } // namespace cyberway::chaindb