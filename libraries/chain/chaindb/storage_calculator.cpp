#include <cyberway/chaindb/storage_calculator.hpp>
#include <cyberway/chaindb/table_info.hpp>

#include <eosio/chain/abi_def.hpp>

#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

#include <fc/io/json.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/permission_object.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::table_def;

    using std::string;

    using fc::variant;
    using fc::variants;
    using fc::variant_object;

    struct path_info final {
        const table_def& table;
        string path;

        path_info(const table_def& table) : table(table) { }
    }; // struct path_info

    struct stack_path final {
        stack_path(path_info* info, const string& key)
        : info_(info) {
            if (!info_) return;
            save_size_ = info_->path.size();

            if (save_size_) info_->path.append(1, '.');
            info_->path.append(key);
        }

        ~stack_path() {
            if (info_) info_->path.resize(save_size_);
        }

        int adjust_elem_size(const int size) const {
            if (!info_) return size;

            auto itr = info_->table.field_index_map.find(info_->path);
            if (info_->table.field_index_map.end() == itr) return size;

            return size * itr->second;
        }

    private:
        path_info* info_ = nullptr;
        string::size_type save_size_ = 0;
    }; // struct stack_path

    int calc_storage_usage(const variant& var, path_info* path);
    int calc_storage_usage(const variant_object& var, path_info* path);

    int calc_storage_usage(const variants& var) {
        constexpr static int base_size = 8; // for length

        int size = base_size;
        for (auto& elem: var) {
            size += calc_storage_usage(elem, nullptr);
        }
        return size;
    }

    int calc_storage_usage(const variant_object& var, path_info* path) {
        constexpr static int base_size = 8;
        constexpr static int size_per_name = 20;  // "name":

        int size = base_size + size_per_name * var.size();
        for (auto& elem: var) {
            auto stack = stack_path(path, elem.key());
            auto elem_size = calc_storage_usage(elem.value(), path);
            size += stack.adjust_elem_size(elem_size);
        }
        return size;
    }

    int calc_storage_usage(const variant& var, path_info* path) {
        constexpr static int base_size = 4;

        switch (var.get_type()) {
            case variant::type_id::null_type:
            case variant::type_id::bool_type:
                return base_size + 4;

            case variant::type_id::double_type:
            case variant::type_id::int64_type:
            case variant::type_id::uint64_type:
            case variant::type_id::time_type:
                return base_size + 8;

            case variant::type_id::int128_type:
            case variant::type_id::uint128_type:
                return base_size + 32;

            case variant::type_id::string_type: {
                auto size = var.get_string().size();
                return ((size >> 4) + 1) << 4;
            }

            case variant::type_id::blob_type:
                return base_size + 2 * var.get_blob().data.size();

            case variant::type_id::array_type:
                return base_size + calc_storage_usage(var.get_array());

            case variant::type_id::object_type:
                return base_size + calc_storage_usage(var.get_object(), path);
        }
        return base_size;
    }

    int get_fixed_storage_usage(const table_info& info, const variant& var) {
        if (is_system_code(info.code)) switch (info.table->name.value) {
            case tag<eosio::chain::account_object>::get_code(): {
                return 736 + var["code"].get_string().size() + var["abi"].get_blob().data.size();
            }

            case tag<eosio::chain::account_sequence_object>::get_code():
                return 460;

            case tag<eosio::chain::permission_link_object>::get_code():
                return 576;

            case tag<eosio::chain::permission_usage_object>::get_code():
                return 360;

            case tag<eosio::chain::block_summary_object>::get_code():
                return 416;

            case tag<eosio::chain::global_property_object>::get_code():
                return 1400;

            case tag<eosio::chain::dynamic_global_property_object>::get_code():
                return 360;

            case tag<eosio::chain::resource_limits::resource_usage_object>::get_code():
                return 796;

            case tag<eosio::chain::resource_limits::resource_limits_config_object>::get_code():
                return 1768;

            case tag<eosio::chain::resource_limits::resource_limits_state_object>::get_code():
                return 1016;
        }
        return 0;
    }

    int calc_storage_usage(const table_info& info, const variant& var) {
        int size = get_fixed_storage_usage(info, var);
        if (size) {
            return size;
        }

        constexpr static int base_size  = 256; /* memory usage of structures in RAM */
        constexpr static int index_size = 12 + 8 /* scope:pk */ + 8; /* pk */

        auto& table = *info.table;

        size = base_size + index_size * table.indexes.size();
        if (table.indexes.size() > 1) {
            auto path = path_info(table);
            size += calc_storage_usage(var, &path);
        } else {
            size += calc_storage_usage(var, nullptr);
        }
        return size;
    }

} } // namespace cyberway::chaindb