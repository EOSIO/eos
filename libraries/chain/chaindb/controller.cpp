#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/symbol.hpp>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/mongo_driver.hpp>

#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::name;
    using eosio::chain::symbol;
    using eosio::chain::struct_def;
    using eosio::chain::abi_serializer;
    using eosio::chain::type_name;
    using eosio::chain::code_type;

    using fc::variant;
    using fc::variants;
    using fc::variant_object;
    using fc::mutable_variant_object;
    using fc::time_point;

    using std::vector;

    using eosio::chain::abi_serialization_deadline_exception;

    std::ostream& operator<<(std::ostream& osm, const chaindb_type t) {
        switch (t) {
            case chaindb_type::MongoDB:
                osm << "MongoDB";
                break;

            default:
                osm << "_UNKNOWN_";
                break;
        }
        return osm;
    }

    std::istream& operator>>(std::istream& in, chaindb_type& type) {
        std::string s;
        in >> s;
        boost::algorithm::to_lower(s);
        if (s == "mongodb") {
            type = chaindb_type::MongoDB;
        } else {
            in.setstate(std::ios_base::failbit);
        }
        return in;
    }

    driver_interface::~driver_interface() = default;

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

        const vector<string>& get_reserved_fields() {
            static const vector<string> fields = {get_scope_field_name(), get_payer_field_name(), get_size_field_name()};
            return fields;
        }

        template <typename Info>
        const index_def& get_pk_index(const Info& info) {
            // abi structure is already validated by abi_serializer
            return info.table->indexes.front();
        }

        template <typename Info>
        const order_def& get_pk_order(const Info& info) {
            // abi structure is already validated by abi_serializer
            return get_pk_index(info).orders.front();
        }

        variant get_pk_value(const table_info& table, const primary_key_t pk) {
            auto& pk_order = *table.pk_order;

            variant value;
            switch (pk_order.type.front()) {
                case 'i': // int64
                case 'u': // uint64
                    value = variant(pk);
                    break;
                case 'n': // name
                    value = variant(name(pk).to_string());
                    break;
                case 's': // symbol_code
                    value = variant(symbol(pk << 8).name());
                    break;
                default:
                    CYBERWAY_ASSERT(false, primary_key_exception, "Invalid type of primary key");
            }

            for (auto itr = pk_order.path.rbegin(), etr = pk_order.path.rend(); etr != itr; ++itr) {
                value = variant(mutable_variant_object(*itr, value));
            }
            return value;
        }

    } } // namespace _detail

    class index_builder final {
        using table_map_type = fc::flat_map<code_type, table_def>;
        index_info info_;
        abi_serializer& serializer_;
        table_map_type& table_map_;

    public:
        index_builder(const account_name& code, abi_serializer& serializer, table_map_type& table_map)
        : info_(code, code),
          serializer_(serializer),
          table_map_(table_map)
        { }

        void build_indexes(const time_point& deadline, const microseconds& max_time) {
            for (auto& t: table_map_) { try {
                CYBERWAY_ASSERT(time_point::now() < deadline, abi_serialization_deadline_exception,
                    "serialization time limit ${t}us exceeded", ("t", max_time));

                // if abi was loaded instead of declared
                auto& table = t.second;
                auto& table_struct = serializer_.get_struct(serializer_.resolve_type(table.type));

                for (auto& index: table.indexes) {
                    // if abi was loaded instead of declared
                    if (!index.code) index.code = index.name.value;

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
                CYBERWAY_ASSERT(order.order == "asc" || order.order == "desc", invalid_index_description_exception,
                    "invalid type of index order '${order}' for the ${index}",
                    ("order", order.order)("index", root_struct.name));

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
            CYBERWAY_ASSERT(k.type == "uint64" || k.type == "name" || k.type == "symbol_code", invalid_primary_key_exception,
                "The field ${field} of the table ${table} is declared as primary "
                "and it should has type uint64 or name or symbol_code",
                ("field", k.field)("table", get_table_name(table)));
        }
    }; // struct index_builder

    class abi_info final {
    public:
        abi_info() = default;

        abi_info(const account_name& code, abi_def abi, const time_point& deadline, const microseconds& max_time)
        : code_(code) {
            serializer_.set_abi(abi, max_time);

            for (auto& table: abi.tables) {
                auto code = (!table.code ? table.name.value : table.code);
                table_map_.emplace(code, std::move(table));
            }

            index_builder builder(code, serializer_, table_map_);
            builder.build_indexes(deadline, max_time);
        }

        void verify_tables_structure(
            driver_interface& driver, const time_point& deadline, const microseconds& max_time
        ) const {
            for (auto& t: table_map_) {
                CYBERWAY_ASSERT(time_point::now() < deadline, abi_serialization_deadline_exception,
                    "serialization time limit ${t}us exceeded", ("t", max_time));

                table_info info(code_, code_);
                info.table = &t.second;
                info.abi = this;
                info.pk_order = &_detail::get_pk_order(info);

                driver.verify_table_structure(info, max_time);
            }
        }

        variant to_object(
            const table_info& info, const void* data, const size_t size, const microseconds& max_time
        ) const {
            CYBERWAY_ASSERT(info.table, unknown_table_exception, "NULL table");
            return to_object_("table", [&]{return get_full_table_name(info);}, info.table->type, data, size, max_time);
        }

        variant to_object(
            const index_info& info, const void* data, const size_t size, const microseconds& max_time
        ) const {
            CYBERWAY_ASSERT(info.index, unknown_index_exception, "NULL index");
            auto type = get_full_index_name(info);
            return to_object_("index", [&](){return type;}, type, data, size, max_time);
        }

        bytes to_bytes(
            const table_info& info, const variant& value, const microseconds& max_time
        ) const {
            CYBERWAY_ASSERT(info.table, unknown_table_exception, "NULL table");
            return to_bytes_("table", [&]{return get_full_table_name(info);}, info.table->type, value, max_time);
        }

        void mark_removed() {
            is_removed_ = true;
        }

        bool is_removed() const {
            return is_removed_;
        }

        const table_def* find_table(code_type code) const {
            auto itr = table_map_.find(code);
            if (table_map_.end() != itr) return &itr->second;

            return nullptr;
        }

    private:
        const account_name code_;
        abi_serializer serializer_;
        fc::flat_map<code_type, table_def> table_map_;
        bool is_removed_ = false;

        template<typename Type>
        variant to_object_(
            const char* value_type, Type&& db_type,
            const string& type, const void* data, const size_t size, const microseconds& max_time
        ) const {
            // begin()
            if (nullptr == data || 0 == size) return variant_object();

            fc::datastream<const char*> ds(static_cast<const char*>(data), size);
            auto value = serializer_.binary_to_variant(type, ds, max_time);

//            dlog(
//                "The ${value_type} '${type}': ${value}",
//                ("value_type", value_type)("type", db_type())("value", value));

            CYBERWAY_ASSERT(value.is_object(), invalid_abi_store_type_exception,
                "ABI serializer returns bad type for the ${value_type} for ${type}: ${value}",
                ("value_type", value_type)("type", db_type())("value", value));

            return value;
        }

        template<typename Type>
        bytes to_bytes_(
            const char* value_type, Type&& db_type,
            const string& type, const variant& value, const microseconds& max_time
        ) const {

//            dlog(
//                "The ${value_type} '${type}': ${value}",
//                ("value", value_type)("type", db_type())("value", value));

            CYBERWAY_ASSERT(value.is_object(), invalid_abi_store_type_exception,
                "ABI serializer receive wrong type for the ${value_type} for '${type}': ${value}",
                ("value_type", value_type)("type", db_type())("value", value));

            return serializer_.variant_to_binary(type, value, max_time);
        }
    }; // struct abi_info

    struct chaindb_controller::controller_impl_ final {
        const microseconds max_abi_time;
        std::unique_ptr<driver_interface> driver;
        std::unique_ptr<cache_map> cache;
        std::unique_ptr<undo_stack> undo;

        controller_impl_(const microseconds& max_abi_time, const chaindb_type t, string p)
        : max_abi_time(max_abi_time) {
            switch(t) {
                case chaindb_type::MongoDB:
                    driver = std::make_unique<mongodb_driver>(std::move(p));
                    break;

                default:
                    CYBERWAY_ASSERT(false, unknown_connection_type_exception,
                        "Invalid type of ChainDB connection");
            }
            cache = std::make_unique<cache_map>();
            undo = std::make_unique<undo_stack>(*driver, *cache);
        }

        ~controller_impl_() = default;

        void drop_db() {
            const auto system_abi_itr = abi_map_.find(0);

            assert(system_abi_itr != abi_map_.end());

            auto system_abi = std::move(system_abi_itr->second);
            driver->drop_db();
            abi_map_.clear();
            set_abi(0, std::move(system_abi), time_point::now() + max_abi_time);
        }

        void set_abi(const account_name& code, abi_def abi) {
            time_point deadline = time_point::now() + max_abi_time;
            abi_info info(code, std::move(abi), deadline, max_abi_time);
            set_abi(code, std::move(info), deadline);
        }

        void remove_abi(const account_name& code) {
            auto itr = abi_map_.find(code);
            if (abi_map_.end() == itr) return;

            itr->second.mark_removed();
        }

        bool has_abi(const account_name& code) const {
            auto itr = abi_map_.find(code);
            if (abi_map_.end() == itr) return false;

            return !itr->second.is_removed();
        }

        const cursor_info& lower_bound(const index_request& request, const char* key, const size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size, max_abi_time);
            return driver->lower_bound(index, std::move(value));
        }

        const cursor_info& upper_bound(const index_request& request, const char* key, const size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size, max_abi_time);
            return driver->upper_bound(index, std::move(value));
        }

        const cursor_info& find(const index_request& request, primary_key_t pk, const char* key, size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size, max_abi_time);
            return driver->find(index, pk, std::move(value));
        }

        const cursor_info& end(const index_request& request) const {
            auto index = get_index(request);
            return driver->end(index);
        }

        primary_key_t available_pk(const table_request& request) const {
            auto table = get_table<table_info>(request);
            return driver->available_pk(table);
        }

        primary_key_t get_next_pk(const table_info& table) const {
            auto next_pk = cache->get_next_pk(table);
            if (next_pk == unset_primary_key) {
                next_pk = driver->available_pk(table);
                cache->set_next_pk(table, next_pk + 1);
            }
            return next_pk;
        }

        int32_t datasize(const cursor_request& request) const {
            auto& cursor = driver->current(request);
            init_cursor_blob(cursor);
            return static_cast<int32_t>(cursor.blob.size());
        }

        const cursor_info& data(const cursor_request& request, const char* data, const size_t size) const {
            auto& cursor = driver->current(request);
            init_cursor_blob(cursor);
            CYBERWAY_ASSERT(cursor.blob.size() == size, invalid_data_size_exception,
                "Wrong data size (${data_size} != ${object_size}) for the table ${table}",
                ("data_size", size)("object_size", cursor.blob.size())("table", get_full_table_name(cursor.index)));

            ::memcpy(const_cast<char*>(data), cursor.blob.data(), cursor.blob.size());
            return cursor;
        }

        cache_item_ptr create_cache_item(const table_request& request, const cache_converter_interface& converter) {
            auto table = get_table<table_info>(request);
            auto next_pk = get_next_pk(table);
            return std::make_shared<cache_item>(next_pk, converter);
        }

        cache_item_ptr get_cache_item(
            const cursor_request& cursor_req, const table_request& table_req,
            const primary_key_t pk, const cache_converter_interface& converter
        ) {
            auto table = get_table<table_info>(table_req);

            auto item = cache->find(table, pk);
            if (!item) {
                auto& cursor = driver->current(cursor_req);
                auto value = driver->value(cursor);
                item = std::make_shared<cache_item>(pk, converter);
                item->convert_variant(value);
                cache->insert(table, item);
            }
            return item;
        }

        const cursor_info& insert(
            apply_context&, const table_request& request, const account_name& payer,
            const primary_key_t pk, const char* data, const size_t size
        ) const {
            auto info = get_table<index_info>(request);
            auto& table = static_cast<const table_info&>(info);

            auto value = table.abi->to_object(table, data, size, max_abi_time);
            auto& cursor = insert(info, std::move(value), payer, pk, size);

            // TODO: update RAM usage

            return cursor;
        }

        const cursor_info& insert(
            const table_request& request, cache_item_ptr item, variant value, const size_t size
        ) const {
            auto info = get_table<index_info>(request);
            auto pk = item->pk;
            cache->insert(info, std::move(item));
            return insert(info, std::move(value), 0, pk, size);
        }

        const cursor_info& insert(
            index_info& info, variant raw_value,
            const account_name& payer, const primary_key_t pk, const size_t size
        ) const {
            auto& table = static_cast<const table_info&>(info);
            auto value = add_service_fields(table, std::move(raw_value), payer, pk, size);

            auto inserted_pk = driver->insert(table, pk, value);
            CYBERWAY_ASSERT(inserted_pk == pk, driver_primary_key_exception,
                "Driver returns ${ipk} instead of ${pk} on inserting into the table ${table}",
                ("ipk", inserted_pk)("pk", pk)("table", get_full_table_name(table)));

            if (undo->enabled()) {
                undo->insert(table, pk);
            }

            info.index = &_detail::get_pk_index(table);

            auto pk_key_value = _detail::get_pk_value(table, pk);

            return driver->opt_find_by_pk(info, pk, std::move(pk_key_value));
        }

        primary_key_t update(
            apply_context&, const table_request& request, account_name payer,
            const primary_key_t pk, const char* data, const size_t size
        ) const {
            auto table = get_table<table_info>(request);
            auto value = table.abi->to_object(table, data, size, max_abi_time);
            auto updated_pk = update(table, std::move(value), payer, pk, size);

            // TODO: update RAM usage

            return updated_pk;
        }

        primary_key_t update(
            const table_request& request, const primary_key_t pk, variant value, const size_t size
        ) const {
            auto table = get_table<table_info>(request);
            return update(table, std::move(value), 0, pk, size);
        }

        primary_key_t update(
            const table_info& table, variant raw_value,
            account_name payer, const primary_key_t pk, const size_t size
        ) const {
            auto orig_value = driver->value(table, pk);
            validate_object(table, orig_value, pk);
            auto& orig_object = orig_value.get_object();

            if (payer.empty()) {
                payer = orig_object[get_payer_field_name()].as_string();
            }

            if (undo->enabled()) {
                undo->update(table, pk, std::move(orig_value));
            }

            auto value = add_service_fields(table, std::move(raw_value), payer, pk, size);

            auto updated_pk = driver->update(table, pk, value);
            CYBERWAY_ASSERT(updated_pk == pk, driver_primary_key_exception,
                "Driver returns ${upk} instead of ${pk} on updating of the table ${table}",
                ("upk", updated_pk)("pk", pk)("table", get_full_table_name(table)));

            return updated_pk;
        }

        primary_key_t remove(apply_context&, const table_request& request, primary_key_t pk) const {
            auto table = get_table<table_info>(request);
            auto removed_pk = remove(table, pk);

            // TODO: update RAM usage

            return removed_pk;
        }

        primary_key_t remove(const table_request& request, primary_key_t pk) const {
            auto table = get_table<table_info>(request);
            cache->remove(table, pk);
            return remove(table, pk);
        }

        primary_key_t remove(const table_info& table, primary_key_t pk) const {
            auto orig_value = driver->value(table, pk);
            validate_object(table, orig_value, pk);
            auto& orig_object = orig_value.get_object();

            if (undo->enabled()) {
                undo->remove(table, pk, std::move(orig_value));
            }

            auto deleted_pk = driver->remove(table, pk);
            CYBERWAY_ASSERT(deleted_pk == pk, driver_primary_key_exception,
                "Driver returns ${dpk} instead of ${pk} on deleting of the table ${table}",
                ("dpk", deleted_pk)("pk", pk)("table", get_full_table_name(table)));

            return deleted_pk;
        }

        variant value_by_pk(const table_request& request, primary_key_t pk) {
            auto table = get_table<table_info>(request);
            return driver->value(table, pk);
        }

        variant value_at_cursor(const cursor_request& request) {
            auto& cursor = driver->current(request);
            return driver->value(cursor);
        }

    private:
        std::map<account_name /* code */, abi_info> abi_map_;

        template <typename Info, typename Request>
        Info get_table(const Request& request) const {
            auto info = find_table<Info>(request);
            CYBERWAY_ASSERT(info.table, unknown_table_exception,
                "ABI table ${table} doesn't exists", ("table", get_full_table_name(request)));

            return info;
        }

        index_info get_index(const index_request& request) const {
            auto info = find_index(request);
            CYBERWAY_ASSERT(info.index, unknown_index_exception,
                "ABI index ${index} doesn't exists", ("index", get_full_index_name(request)));

            return info;
        }

        template <typename Info, typename Request>
        Info find_table(const Request& request) const {
            account_name code(request.code);
            account_name scope(request.scope);
            Info info(code, scope);

            auto itr = abi_map_.find(code);
            if (abi_map_.end() == itr) return info;

            info.abi = &itr->second;
            info.table = itr->second.find_table(request.table);
            if (info.table) info.pk_order = &_detail::get_pk_order(info);
            return info;
        }

        index_info find_index(const index_request& request) const {
            auto info = find_table<index_info>(request);
            if (info.table == nullptr) return info;

            for (auto& i: info.table->indexes) {
                if (i.code == request.index) {
                    info.index = &i;
                    break;
                }
            }
            return info;
        }

        void init_cursor_blob(const cursor_info& cursor) const {
            CYBERWAY_ASSERT(cursor.index.abi != nullptr && cursor.index.table != nullptr, broken_driver_exception,
                "Driver returns bad information about abi.");

            if (!cursor.blob.empty()) return;

            auto& value = driver->value(cursor);
            validate_object(cursor.index, value, cursor.pk);

            auto buffer = cursor.index.abi->to_bytes(cursor.index, value, max_abi_time);
            driver->set_blob(cursor, std::move(buffer));
        }

        template <typename Exception>
        void validate_pk_field(const table_info& table, variant value, const primary_key_t pk) const { try {
            auto& pk_order = *table.pk_order;
            for (auto& key: pk_order.path) {
                CYBERWAY_ASSERT(value.is_object(), Exception, "Wrong value type");

                auto& object = value.get_object();
                auto itr = object.find(key);
                CYBERWAY_ASSERT(object.end() != itr, Exception, "Wrong path");

                value = (*itr).value();
            }

            auto type = value.get_type();
            switch(pk_order.type.front()) {
                case 'i':   // int64
                case 'u': { // uint64
                    CYBERWAY_ASSERT(variant::type_id::uint64_type == type || variant::type_id::int64_type == type, Exception, "Wrong value type");
                    CYBERWAY_ASSERT(pk == value.as_uint64(), Exception, "Wrong value");
                    break;
                }
                case 'n': { // name
                    CYBERWAY_ASSERT(variant::type_id::string_type == type, Exception, "Wrong value type");
                    CYBERWAY_ASSERT(name(value.as_string()).value == pk, Exception, "Wrong value");
                    break;
                }
                case 's': { // symbol_code
                    CYBERWAY_ASSERT(variant::type_id::string_type == type, Exception, "Wrong value type ");
                    CYBERWAY_ASSERT(symbol(0, value.as_string().c_str()).to_symbol_code() == pk, Exception, "Wrong value");
                    break;
                }
                default:
                    CYBERWAY_ASSERT(false, Exception, "Invalid type of primary key");
            }
        } catch (...) {
            CYBERWAY_ASSERT(false, Exception,
                "The table ${table} has wrong value of primary key", ("table", get_full_table_name(table)));
        } }

        void validate_object(const table_info& table, const variant& value, const primary_key_t pk) const {
            CYBERWAY_ASSERT(value.is_object(), broken_driver_exception, "ChainDB driver returns not object.");
            auto& object = value.get_object();

            auto& fields = _detail::get_reserved_fields();
            for (auto& field: fields) {
                CYBERWAY_ASSERT(object.end() != object.find(field), driver_absent_field_exception,
                    "ChainDB driver returns object without field ${name} for the table ${table}.",
                    ("table", get_full_table_name(table))("name", field));
            }

            validate_pk_field<driver_primary_key_exception>(table, value, pk);
        }

        void validate_service_fields(const table_info& table, const variant& value, const primary_key_t pk) const {
            // type is object - should be checked before this call
            auto& object = value.get_object();

            auto& fields = _detail::get_reserved_fields();
            for (auto& field: fields) {
                CYBERWAY_ASSERT(object.end() == object.find(field), reserved_field_name_exception,
                    "The table ${table} can't use ${name} as field name.",
                    ("table", get_full_table_name(table))("name", field));
            }
            validate_pk_field<primary_key_exception>(table, value, pk);
        }

        variant add_service_fields(
            const table_info& table, variant value, const account_name& payer, const primary_key_t pk, size_t size
        ) const {
            using namespace _detail;

            validate_service_fields(table, value, pk);

            mutable_variant_object object(std::move(value));
            object(get_scope_field_name(), get_scope_name(table));
            object(get_payer_field_name(), get_payer_name(payer));
            object(get_size_field_name(), size);

            return variant(std::move(object));
        }

        void set_abi(const account_name& code, abi_info&& info, time_point deadline) {
            info.verify_tables_structure(*driver.get(), deadline, max_abi_time);
            abi_map_.erase(code);
            abi_map_.emplace(code, std::forward<abi_info>(info));
        }
    }; // class chaindb_controller::controller_impl_

    chaindb_controller::chaindb_controller(const microseconds& max_abi_time, const chaindb_type t, string p)
    : impl_(new controller_impl_(max_abi_time, t, std::move(p))) {
    }

    chaindb_controller::~chaindb_controller() = default;

    void chaindb_controller::drop_db() {
        impl_->drop_db();
    }

    void chaindb_controller::add_abi(const account_name& code, abi_def abi) {
        impl_->set_abi(code, std::move(abi));
    }

    void chaindb_controller::remove_abi(const account_name& code) {
        impl_->remove_abi(code);
    }

    bool chaindb_controller::has_abi(const account_name& code) {
        return impl_->has_abi(code);
    }

    chaindb_session chaindb_controller::start_undo_session(bool enabled) {
        return impl_->undo->start_undo_session(enabled);
    }

    void chaindb_controller::undo() {
        return impl_->undo->undo();
    }

    void chaindb_controller::undo_all() {
        return impl_->undo->undo_all();
    }

    void chaindb_controller::commit(const int64_t revision) {
        return impl_->undo->commit(revision);
    }

    int64_t chaindb_controller::revision() const {
        return impl_->undo->revision();
    }
    void chaindb_controller::set_revision(uint64_t revision) {
        return impl_->undo->set_revision(revision);
    }

    void chaindb_controller::close(const cursor_request& request) {
        impl_->driver->close(request);
    }

    void chaindb_controller::close_all_cursors(const account_name& code) {
        impl_->driver->close_all_cursors(code);
    }

    void chaindb_controller::apply_changes(const account_name& code) {
        impl_->driver->apply_changes(code);
    }

    cursor_t chaindb_controller::lower_bound(const index_request& request, const char* key, size_t size) {
        return impl_->lower_bound(request, key, size).id;
    }

    cursor_t chaindb_controller::upper_bound(const index_request& request, const char* key, size_t size) {
        return impl_->upper_bound(request, key, size).id;
    }

    cursor_t chaindb_controller::find(const index_request& request, primary_key_t pk, const char* key, size_t size) {
        return impl_->find(request, pk, key, size).id;
    }

    cursor_t chaindb_controller::end(const index_request& request) {
        return impl_->end(request).id;
    }

    cursor_t chaindb_controller::clone(const cursor_request& request) {
        return impl_->driver->clone(request).id;
    }

    primary_key_t chaindb_controller::current(const cursor_request& request) {
        return impl_->driver->current(request).pk;
    }

    primary_key_t chaindb_controller::next(const cursor_request& request) {
        return impl_->driver->next(request).pk;
    }

    primary_key_t chaindb_controller::prev(const cursor_request& request) {
        return impl_->driver->prev(request).pk;
    }

    int32_t chaindb_controller::datasize(const cursor_request& request) {
        return impl_->datasize(request);
    }

    primary_key_t chaindb_controller::data(const cursor_request& request, const char* data, size_t size) {
        return impl_->data(request, data, size).pk;
    }

    cache_item_ptr chaindb_controller::create_cache_item(
        const table_request& table, const cache_converter_interface& converter
    ) {
        return impl_->create_cache_item(table, converter);
    }

    cache_item_ptr chaindb_controller::get_cache_item(
        const cursor_request& cursor, const table_request& table,
        const primary_key_t pk, const cache_converter_interface& converter
    ) {
        return impl_->get_cache_item(cursor, table, pk, converter);
    }

    primary_key_t chaindb_controller::available_pk(const table_request& request) {
        return impl_->available_pk(request);
    }

    cursor_t chaindb_controller::insert(
        apply_context& ctx, const table_request& request, const account_name& payer,
        primary_key_t pk, const char* data, size_t size
    ) {
        return impl_->insert(ctx, request, payer, pk, data, size).id;
    }

    primary_key_t chaindb_controller::update(
        apply_context& ctx, const table_request& request, const account_name& payer,
        primary_key_t pk, const char* data, size_t size
    ) {
        return impl_->update(ctx, request, payer, pk, data, size);
    }

    primary_key_t chaindb_controller::remove(apply_context& ctx, const table_request& request, primary_key_t pk) {
        return impl_->remove(ctx, request, pk);
    }

    cursor_t chaindb_controller::insert(
        const table_request& request, cache_item_ptr item, variant data, size_t size
    ) {
        return impl_->insert(request, item, std::move(data), size).id;
    }

    primary_key_t chaindb_controller::update(
        const table_request& request, primary_key_t pk, variant data, size_t size
    ) {
        return impl_->update(request, pk, std::move(data), size);
    }

    primary_key_t chaindb_controller::remove(const table_request& request, primary_key_t pk) {
        return impl_->remove(request, pk);
    }

    variant chaindb_controller::value_by_pk(const table_request& request, primary_key_t pk) {
        return impl_->value_by_pk(request, pk);
    }

    variant chaindb_controller::value_at_cursor(const cursor_request& request) {
        return impl_->value_at_cursor(request);
    }

} } // namespace cyberway::chaindb
