#include <eosio/chain/abi_serializer.hpp>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/mongo_driver.hpp>

#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::struct_def;
    using eosio::chain::field_def;
    using eosio::chain::abi_serializer;

    using fc::variant;
    using fc::variants;
    using fc::variant_object;
    using fc::mutable_variant_object;
    using fc::time_point;

    using std::vector;

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
        const vector<string>& reserved_fields() {
            static const vector<string> fields = {scope_field_name(), payer_field_name(), size_field_name()};
            return fields;
        }
    } } // namespace _detail

    class abi_info {
    public:
        abi_info() = default;
        abi_info(const account_name& code, abi_def abi, const microseconds& max_time)
        : code_(code),
          abi_(std::move(abi)) {

            serializer_.set_abi(abi, max_time);

            for (auto& table: abi.tables) {
                // if abi was loaded instead of declared
                if (!table.code) table.code = table.name.value;
                auto& src = serializer_.get_struct(serializer_.resolve_type(table.type));
                for (auto& index: table.indexes) {
                    // if abi was loaded instead of declared
                    if (!index.code) index.code = index.name.value;

                    index_info i(code, code);
                    i.table = &table;
                    i.index = &index;

                    struct_def dst;
                    dst.name = full_index_name(i);
                    for (auto& key: index.orders) {
                        for (auto& field: src.fields) {
                            if (field.name == key.field) {
                                dst.fields.push_back(field);
                                break;
                            }
                        }
                    }

                    abi_.structs.push_back(dst);
                    serializer_.add_struct(std::move(dst));
                }
            }
        }

        void verify_tables_structure(
            driver_interface& driver, const time_point& deadline, const microseconds& max_time
        ) const {
            for (auto& t: abi_.tables) {
                table_info info(code_, code_);
                info.table = &t;
                info.abi = this;

                driver.verify_table_structure(info, max_time);

                CYBERWAY_ASSERT(time_point::now() < deadline, eosio::chain::abi_serialization_deadline_exception,
                    "serialization time limit ${t}us exceeded", ("t", max_time) );
            }
        }

        variant to_object(
            const table_info& info, const void* data, const size_t size, const microseconds& max_time
        ) const {
            CYBERWAY_ASSERT(info.table, unknown_table_exception, "NULL table");
            return to_object_("table", [&]{return full_table_name(info);}, info.table->type, data, size, max_time);
        }

        variant to_object(
            const index_info& info, const void* data, const size_t size, const microseconds& max_time
        ) const {
            CYBERWAY_ASSERT(info.index, unknown_index_exception, "NULL index");
            auto type = full_index_name(info);
            return to_object_("index", [&](){return type;}, type, data, size, max_time);
        }

        bytes to_bytes(
            const table_info& info, const variant& value, const microseconds& max_time
        ) const {
            CYBERWAY_ASSERT(info.table, unknown_table_exception, "NULL table");
            return to_bytes_("table", [&]{return full_table_name(info);}, info.table->type, value, max_time);
        }

        void mark_removed() {
            is_removed_ = true;
        }

        bool is_removed() const {
            return is_removed_;
        }

        const abi_def& abi() const {
            return abi_;
        }

    private:
        const account_name code_;
        abi_def abi_;
        abi_serializer serializer_;
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

            ilog(
                "The ${value_type} '${type}': ${value}",
                ("value_type", value_type)("type", db_type())("value", value));

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

            ilog(
                "The ${value_type} '${type}': ${value}",
                ("value", value_type)("type", db_type())("value", value));

            CYBERWAY_ASSERT(value.is_object(), invalid_abi_store_type_exception,
                "ABI serializer receive wrong type for the ${value_type} for '${type}': ${value}",
                ("value_type", value_type)("type", db_type())("value", value));

            return serializer_.variant_to_binary(type, value, max_time);
        }
    }; // struct abi_info

    struct chaindb_controller::controller_impl_ {
        const microseconds max_abi_time;
        std::unique_ptr<driver_interface> driver;

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
        }

        ~controller_impl_() = default;

        void set_abi(const account_name& code, abi_def abi) {
            time_point deadline = time_point::now() + max_abi_time;
            abi_info info(code, std::move(abi), max_abi_time);
            info.verify_tables_structure(*driver.get(), deadline, max_abi_time);
            abi_map_.erase(code);
            abi_map_.emplace(code, std::move(info));
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

        primary_key_t available_primary_key(const table_request& request) const {
            auto table = get_table<table_info>(request);
            return driver->available_primary_key(table);
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
                ("data_size", size)("object_size", cursor.blob.size())("table", full_table_name(cursor.index)));

            ::memcpy(const_cast<char*>(data), cursor.blob.data(), cursor.blob.size());
            return cursor;
        }

        const cursor_info& insert(
            apply_context&, const table_request& request, const account_name& payer,
            const primary_key_t pk, const char* data, const size_t size
        ) const {

            auto info = get_table<index_info>(request);
            auto& table = static_cast<const table_info&>(info);

            auto raw_value = table.abi->to_object(table, data, size, max_abi_time);
            auto value = add_service_fields(table, payer, raw_value, pk, size);

            auto inserted_pk = driver->insert(table, std::move(value));
            CYBERWAY_ASSERT(inserted_pk == pk, driver_primary_key_exception,
                "Driver returns ${ipk} instead of ${pk} on inserting into the table ${table}",
                ("ipk", inserted_pk)("pk", pk)("table", full_table_name(table)));

            // open cursor from inserted pk
            mutable_variant_object key_object;
            key_object(*table.pk_field, pk);
            variant key(std::move(key_object));

            info.index = &get_pk_index(table);

            auto& cursor = driver->lower_bound(info, std::move(key));
            CYBERWAY_ASSERT(cursor.pk == pk, driver_primary_key_exception,
                "Driver returns ${ipk} instead of ${pk} on loading from the table ${table}",
                ("ipk", cursor.pk)("pk", pk)("table", full_table_name(table)));

            // TODO: update RAM usage
            // TODO: add to undo_session
            return cursor;
        }

        primary_key_t update(
            apply_context&, const table_request& request, account_name payer,
            const primary_key_t pk, const char* data, size_t size
        ) const {

            auto table = get_table<table_info>(request);

            auto raw_value = table.abi->to_object(table, data, size, max_abi_time);

            auto orig_value = driver->value(table, pk);
            validate_object(table, orig_value, pk);
            auto& orig_object = orig_value.get_object();

            if (payer.empty()) {
                payer = orig_object[payer_field_name()].as_string();
            }

            auto value = add_service_fields(table, payer, raw_value, pk, size);

            auto updated_pk = driver->update(table, std::move(value));
            CYBERWAY_ASSERT(updated_pk == pk, driver_primary_key_exception,
                "Driver returns ${upk} instead of ${pk} on updating of the table ${table}",
                ("upk", updated_pk)("pk", pk)("table", full_table_name(table)));

            // TODO: update RAM usage
            // TODO: add to undo_session

            return updated_pk;
        }

        primary_key_t remove(apply_context&, const table_request& request, primary_key_t pk) const {
            auto table = get_table<table_info>(request);

            auto orig_value = driver->value(table, pk);
            validate_object(table, orig_value, pk);
            auto& orig_object = orig_value.get_object();

            auto deleted_pk = driver->remove(table, pk);
            CYBERWAY_ASSERT(deleted_pk == pk, driver_primary_key_exception,
                "Driver returns ${dpk} instead of ${pk} on deleting of the table ${table}",
                ("dpk", deleted_pk)("pk", pk)("table", full_table_name(table)));

            // TODO: update RAM usage
            // TODO: add to undo_session

            return deleted_pk;
        }

    private:
        std::map<account_name /* code */, abi_info> abi_map_;

        template <typename Info, typename Request>
        Info get_table(const Request& request) const {
            auto info = find_table<Info>(request);
            CYBERWAY_ASSERT(info.table, unknown_table_exception,
                "ABI table ${table} doesn't exists",
                ("table", full_table_name(request)));

            return info;
        }

        index_info get_index(const index_request& request) const {
            auto info = find_index(request);
            CYBERWAY_ASSERT(info.index, unknown_index_exception,
                "ABI index ${index} doesn't exists",
                ("index", full_index_name(request)));

            return info;
        }

        template <typename Info>
        const index_def& get_pk_index(const Info& info) const {
            // abi structure is already validated by abi_serializer
            return info.table->indexes.front();
        }

        template <typename Info>
        const field_name& get_pk_field(const Info& info) const {
            // abi structure is already validated by abi_serializer
            return get_pk_index(info).orders.front().field;
        }

        template <typename Info, typename Request>
        Info find_table(const Request& request) const {
            account_name code(request.code);
            account_name scope(request.scope);
            Info info(code, scope);

            auto itr = abi_map_.find(code);
            if (abi_map_.end() == itr) return info;

            info.abi = &itr->second;

            auto& tables = itr->second.abi().tables;
            for (auto& t: tables) {
                if (t.code == request.table) {
                    info.table = &t;
                    info.pk_field = &get_pk_field(info);
                    break;
                }
            }
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

        void validate_object(const table_info& table, const variant& value, const primary_key_t pk) const {
            CYBERWAY_ASSERT(value.is_object(), broken_driver_exception,
                "ChainDB driver returns not object.");
            auto& object = value.get_object();

            auto& fields = _detail::reserved_fields();
            for (auto& field: fields) {
                CYBERWAY_ASSERT(object.end() != object.find(field), driver_absent_field_exception,
                    "ChainDB driver returns object without field ${name} for the table ${table}.",
                    ("table", full_table_name(table))("name", field));
            }

            auto itr = object.find(*table.pk_field);
            CYBERWAY_ASSERT(object.end() != itr, driver_primary_key_exception,
                "ChainDB driver returns object without primary key", ("table", full_table_name(table)));

            CYBERWAY_ASSERT(variant::uint64_type == itr->value().get_type(), driver_primary_key_exception,
                "ChainDB driver returns object with wrong type of primary key", ("table", full_table_name(table)));

            CYBERWAY_ASSERT(pk == itr->value().as_uint64(), driver_primary_key_exception,
                "ChainDB driver returns object with wrong value of primary key", ("table", full_table_name(table)));
        }

        void validate_service_fields(const table_info& table, const variant& value, const primary_key_t pk) const {
            // type is object - should be checked before this call
            auto& object = value.get_object();
            auto& fields = _detail::reserved_fields();

            for (auto& field: fields) {
                CYBERWAY_ASSERT(object.end() == object.find(field), reserved_field_name_exception,
                    "The table ${table} can't use ${name} as field name.",
                    ("table", full_table_name(table))("name", field));
            }

            auto itr = object.find(*table.pk_field);
            CYBERWAY_ASSERT(object.end() != itr, primary_key_exception,
                "The table ${table} doesn't have primary key", ("table", full_table_name(table)));

            CYBERWAY_ASSERT(variant::uint64_type == itr->value().get_type(), primary_key_exception,
                "The table ${table} has wrong type of primary key", ("table", full_table_name(table)));

            CYBERWAY_ASSERT(pk == itr->value().as_uint64(), primary_key_exception,
                "The table ${table} has wrong value of primary key", ("table", full_table_name(table)));
        }

        variant add_service_fields(
            const table_info& table,
            const account_name& payer, const variant& value, const primary_key_t pk, size_t size
        ) const {
            using namespace _detail;

            validate_service_fields(table, value, pk);

            mutable_variant_object object(value);
            object(scope_field_name(), scope_name(table));
            object(payer_field_name(), payer_name(payer));
            object(size_field_name(), size);

            return variant(std::move(object));
        }
    }; // class chaindb_controller::controller_impl_

    chaindb_controller::chaindb_controller(const microseconds& max_abi_time, const chaindb_type t, string p)
    : impl_(new controller_impl_(max_abi_time, t, std::move(p))) {
    }

    chaindb_controller::~chaindb_controller() = default;

    void chaindb_controller::add_abi(const account_name& code, abi_def abi) {
        impl_->set_abi(code, std::move(abi));
    }

    void chaindb_controller::remove_abi(const account_name& code) {
        impl_->remove_abi(code);
    }

    bool chaindb_controller::has_abi(const account_name& code) {
        return impl_->has_abi(code);
    }

    void chaindb_controller::close(const cursor_request& request) {
        impl_->driver->close(request);
    }

    void chaindb_controller::close_all_cursors(const account_name& code) {
        impl_->driver->close_all_cursors(code);
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

    primary_key_t chaindb_controller::available_primary_key(const table_request& request) {
        return impl_->available_primary_key(request);
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

} } // namespace cyberway::chaindb