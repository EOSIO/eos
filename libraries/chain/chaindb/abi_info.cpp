#include <cyberway/chaindb/abi_info.hpp>

#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::struct_def;
    using eosio::chain::type_name;

    namespace { namespace _detail {

        using struct_def_map_type = fc::flat_map<type_name, struct_def>;

        const struct_def_map_type& get_system_types() {
            static auto types = [](){
                struct_def_map_type init_types;

                init_types.emplace("asset", struct_def{
                    "asset", "", {
                        {"amount", "uint64"},
                        {"decs",   "uint8"},
                        {"sym",    "symbol_code"},
                    }
                });

                init_types.emplace("symbol", struct_def{
                    "symbol", "", {
                        {"decs", "uint8"},
                        {"sym",  "symbol_code"},
                    }
                });

                return init_types;
            }();

            return types;
        }
    } }

    class index_builder final {
        using table_map_type = fc::flat_map<hash_t, table_def>;
        index_info info_;
        abi_serializer& serializer_;
        table_map_type& table_map_;

    public:
        index_builder(const account_name& code, abi_serializer& serializer, table_map_type& table_map)
        : info_(code, code),
          serializer_(serializer),
          table_map_(table_map) {
        }

        void build_indexes(const time_point& deadline, const microseconds& max_time) {
            for (auto& t: table_map_) { try {
// TODO: Limitation by timeout
//                    CYBERWAY_ASSERT(time_point::now() < deadline, abi_serialization_deadline_exception,
//                        "serialization time limit ${t}us exceeded", ("t", max_time));

                    // if abi was loaded instead of declared
                    auto& table = t.second;
                    auto& table_struct = serializer_.get_struct(serializer_.resolve_type(table.type));

                    for (auto& index: table.indexes) {
                        // if abi was loaded instead of declared
                        if (!index.hash) index.hash = index.name.value;

                        build_index(table, index, table_struct);
                    }
                    validate_pk_index(table);

                } FC_CAPTURE_AND_RETHROW((t.second)) }
        }

    private:
        const struct_def& get_struct(const type_name& type) const {
            auto resolved_type = serializer_.resolve_type(type);

            auto& system_map = _detail::get_system_types();
            auto itr = system_map.find(resolved_type);
            if (system_map.end() != itr) {
                return itr->second;
            }

            return serializer_.get_struct(resolved_type);
        }

        type_name get_root_index_name(const table_def& table, const index_def& index) {
            info_.table = &table;
            info_.index = &index;
            return get_full_index_name(info_);
        }

        void build_index(table_def& table, index_def& index, const struct_def& table_struct) {
            _detail::struct_def_map_type dst_struct_map;

            auto struct_name = get_root_index_name(table, index);
            auto& root_struct = dst_struct_map.emplace(struct_name, struct_def(struct_name, "", {})).first->second;
            for (auto& order: index.orders) {
                CYBERWAY_ASSERT(order.order == names::asc_order || order.order == names::desc_order,
                    invalid_index_description_exception,
                    "Invalid type '${type}' for the index order for the index ${index}",
                    ("type", order.order)("index", root_struct.name));

                boost::split(order.path, order.field, [](char c){return c == '.';});

                auto dst_struct = &root_struct;
                auto src_struct = &table_struct;
                auto size = order.path.size();
                for (auto& key: order.path) {
                    for (auto& src_field: src_struct->fields) {
                        if (src_field.name != key) continue;

                        --size;
                        if (!size) {
                            auto field = src_field;
                            field.type = serializer_.resolve_type(src_field.type);
                            order.type = field.type;
                            dst_struct->fields.emplace_back(std::move(field));
                        } else {
                            src_struct = &get_struct(src_field.type);

                            struct_name.append('.', 1).append(key);
                            auto itr = dst_struct_map.find(struct_name);
                            if (dst_struct_map.end() == itr) {
                                dst_struct->fields.emplace_back(key, struct_name);
                                itr = dst_struct_map.emplace(struct_name, struct_def(struct_name, "", {})).first;
                            }
                            dst_struct = &itr->second;
                        }
                        break;
                    }
                }
                CYBERWAY_ASSERT(!size && !order.type.empty(), invalid_index_description_exception,
                    "Can't find type for fields of the index ${index}", ("index", root_struct.name));

                struct_name = root_struct.name;
            }

            for (auto& t: dst_struct_map) {
                serializer_.add_struct(std::move(t.second));
            }
        }

        void validate_pk_index(const table_def& table) const {
            CYBERWAY_ASSERT(!table.indexes.empty(), invalid_primary_key_exception,
                "The table ${table} must contain at least one index", ("table", get_table_name(table)));

            auto& p = table.indexes.front();
            CYBERWAY_ASSERT(p.unique && p.orders.size() == 1, invalid_primary_key_exception,
                "The primary index ${index} of the table ${table} should be unique and has one field",
                ("table", get_table_name(table))("index", get_index_name(p)));

            auto& k = p.orders.front();
            CYBERWAY_ASSERT(k.type == "int64" || k.type == "uint64" || k.type == "name" || k.type == "symbol_code",
                invalid_primary_key_exception,
                "The field ${field} of the table ${table} is declared as primary "
                "and it should has type int64, uint64, name or symbol_code",
                ("field", k.field)("table", get_table_name(table)));
        }
    }; // struct index_builder

    abi_info::abi_info(const account_name& code, abi_def abi, const time_point& deadline, const microseconds& max_time)
    : code_(code) {
        serializer_.set_abi(abi, max_time);

        for (auto& table: abi.tables) {
            if (!table.hash) table.hash = table.name.value;
            table_map_.emplace(table.hash, std::move(table));
        }

        index_builder builder(code, serializer_, table_map_);
        builder.build_indexes(deadline, max_time);
    }

    void abi_info::verify_tables_structure(
        driver_interface& driver, const time_point& deadline, const microseconds& max_time
    ) const {
        for (auto& t: table_map_) {
            table_info info(code_, code_);
            info.table = &t.second;
            info.abi = this;
            info.pk_order = &get_pk_order(info);

            driver.verify_table_structure(info, max_time);
        }
    }

} } // namespace cyberway::chaindb