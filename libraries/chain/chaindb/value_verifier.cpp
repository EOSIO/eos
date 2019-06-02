#include <cyberway/chaindb/value_verifier.hpp>
#include <cyberway/chaindb/table_info.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/account_abi_info.hpp>

namespace cyberway { namespace chaindb {

    struct value_verifier_impl final {

        value_verifier_impl(driver_interface& driver)
        : driver_(driver) {
        }

        void verify(const table_info& table, const object_value& obj) {
            if (is_system_code(table.code)) switch (table.table_name()) {
                case tag<account_object>::get_code():
                    verify_account(obj);
                    break;

                default:
                    break;
            }
        }

    private:
        void verify_account(const object_value& obj) {
            if (is_system_code(obj.pk())) {
                account_abi_info::system_abi_info().abi().verify_tables_structure(driver_);
                return;
            }

            eosio::chain::abi_def def;
            auto& abi_value = obj.value["abi"];
            if (abi_value.is_blob()) {
                auto& blob = abi_value.get_blob().data;

                if (!blob.empty()) {
                    fc::datastream<const char*> ds(blob.data(), blob.size());
                    fc::raw::unpack(ds, def);
                }
            }

            abi_info info(obj.pk(), def);
            info.verify_tables_structure(driver_);
        }

        driver_interface& driver_;
    }; // struct value_verifier_impl

    value_verifier::value_verifier(driver_interface& driver)
    : impl_(std::make_unique<value_verifier_impl>(driver)) {
    }

    value_verifier::~value_verifier() = default;

    void value_verifier::verify(const table_info& table, const object_value& obj) const {
        impl_->verify(table, obj);
    }

}} // namespace cyberway::chaindb


