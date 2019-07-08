#include <cyberway/chaindb/value_verifier.hpp>
#include <cyberway/chaindb/table_info.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/account_abi_info.hpp>
#include <eosio/chain/config.hpp>

namespace cyberway { namespace chaindb {

    struct value_verifier_impl final {
        value_verifier_impl(const chaindb_controller& controller)
        : controller_(controller),
          driver_(controller.get_driver()),
          info_(controller.get_system_abi_info()) {
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
            abi_def def;
            auto& abi_value = obj.value["abi"];
            if (abi_value.is_blob()) {
                auto& blob = abi_value.get_blob();

                if (!blob.data.empty()) {
                    abi_serializer::to_abi(blob.data, def);
                }

                if (obj.pk() == config::system_account_name) {
                    info_.set_abi(def);
                }
            }

            if (start_revision != controller_.revision()) {
                controller_.apply_code_changes(obj.pk());
            }
            abi_info info(obj.pk(), def);
            info.verify_tables_structure(driver_);
        }

        const chaindb_controller& controller_;
        const driver_interface&   driver_;
        const system_abi_info&    info_;
    }; // struct value_verifier_impl

    value_verifier::value_verifier(const chaindb_controller& controller)
    : impl_(std::make_unique<value_verifier_impl>(controller)) {
    }

    value_verifier::~value_verifier() = default;

    void value_verifier::verify(const table_info& table, const object_value& obj) const {
        impl_->verify(table, obj);
    }

}} // namespace cyberway::chaindb


