#include <cyberway/chaindb/storage_calculator.hpp>

#include <eosio/chain/abi_def.hpp>

#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

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

            case variant::type_id::string_type:
                return base_size + var.get_string().size();

            case variant::type_id::blob_type:
                return base_size + 2 * var.get_blob().data.size();

            case variant::type_id::array_type:
                return base_size + calc_storage_usage(var.get_array());

            case variant::type_id::object_type:
                return base_size + calc_storage_usage(var.get_object(), path);
        }
        return base_size;
    }

    int calc_storage_usage(const eosio::chain::table_def& table, const variant& var) {
        constexpr static int base_size = 128; /* "_SERVICE_":{"scope":,"rev":,"payer":,"owner":,"size":,"ram":} */
        constexpr static int index_size = 12 + 8 /* scope:pk */ + 8; /* pk */

        int size = base_size + index_size * table.indexes.size();
        if (table.indexes.size() > 1) {
            auto path = path_info(table);
            size += calc_storage_usage(var, &path);
        } else {
            size += calc_storage_usage(var, nullptr);
        }
        return size;
    }

} } // namespace cyberway::chaindb