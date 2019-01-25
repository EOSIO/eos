#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/object_value.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/abi_info.hpp>

#include <eosio/chain/name.hpp>
#include <eosio/chain/symbol.hpp>

#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::name;
    using eosio::chain::symbol;

    using fc::variant;
    using fc::variants;
    using fc::variant_object;
    using fc::mutable_variant_object;

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
        std::unique_ptr<driver_interface> create_driver(chaindb_type t, journal& jrnl, string p) {
            switch(t) {
                case chaindb_type::MongoDB:
                    return std::make_unique<mongodb_driver>(jrnl, std::move(p));

                default:
                    break;
            }
            CYBERWAY_ASSERT(
                false, unknown_connection_type_exception,
                "Invalid type ${type} of ChainDB connection", ("type", t));
        }

        variant get_pk_value(const table_info& table, const primary_key_t pk) {
            auto& pk_order = *table.pk_order;

            variant value;
            switch (pk_order.type.front()) {
                case 'i': // int64
                    value = variant(static_cast<int64_t>(pk));
                    break;
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
                    CYBERWAY_ASSERT(false, invalid_primary_key_exception,
                        "Invalid type ${type} of the primary key for the table ${table}",
                        ("type", pk_order.type)("table", get_full_table_name(table)));
            }

            for (auto itr = pk_order.path.rbegin(), etr = pk_order.path.rend(); etr != itr; ++itr) {
                value = variant(mutable_variant_object(*itr, value));
            }
            return value;
        }

    } } // namespace _detail

    struct chaindb_controller::controller_impl_ final {
        journal journal_;
        std::unique_ptr<driver_interface> driver_ptr_;
        driver_interface& driver_;
        cache_map cache_;
        undo_stack undo_;

        controller_impl_(chaindb_controller& controller, const chaindb_type t, string p)
        : driver_ptr_(_detail::create_driver(t, journal_, std::move(p))),
          driver_(*driver_ptr_.get()),
          undo_(controller, driver_, journal_, cache_) {
        }

        ~controller_impl_() = default;

        void restore_db() {
            undo_.restore();
        }

        void drop_db() {
            const auto itr = abi_map_.find(0);

            assert(itr != abi_map_.end());

            auto system_abi = std::move(itr->second);
            undo_.clear(); // remove all undo states
            journal_.clear(); // remove all pending changes
            driver_.drop_db(); // drop database
            abi_map_.clear(); // clear ABIes
            set_abi(0, std::move(system_abi));
        }

        void set_abi(const account_name& code, abi_def abi) {
            if (code.empty()) {
                undo_.add_abi_tables(abi);
            }
            abi_info info(code, std::move(abi));
            set_abi(code, std::move(info));
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

        const abi_map& get_abi_map() const {
            return abi_map_;
        }

        const cursor_info& lower_bound(const index_request& request, const char* key, const size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size);
            return driver_.lower_bound(std::move(index), std::move(value));
        }

        const cursor_info& upper_bound(const index_request& request, const char* key, const size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size);
            return driver_.upper_bound(std::move(index), std::move(value));
        }

        const cursor_info& find(const index_request& request, primary_key_t pk, const char* key, size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size);
            return driver_.find(std::move(index), pk, std::move(value));
        }

        const cursor_info& begin(const index_request& request) const {
            auto index = get_index(request);
            return driver_.begin(std::move(index));
        }

        const cursor_info& end(const index_request& request) const {
            auto index = get_index(request);
            return driver_.end(std::move(index));
        }

        primary_key_t available_pk(const table_request& request) const {
            auto table = get_table(request);
            return driver_.available_pk(table);
        }

        int32_t datasize(const cursor_request& request) const {
            auto& cursor = driver_.current(request);
            init_cursor_blob(cursor);
            return static_cast<int32_t>(cursor.blob.size());
        }

        const cursor_info& data(const cursor_request& request, const char* data, const size_t size) const {
            auto& cursor = driver_.current(request);
            init_cursor_blob(cursor);
            CYBERWAY_ASSERT(cursor.blob.size() == size, invalid_data_size_exception,
                "Wrong data size (${data_size} != ${object_size}) for the table ${table} in the scope '${scope}'",
                ("data_size", size)("object_size", cursor.blob.size())
                ("table", get_full_table_name(cursor.index))("scope", get_scope_name(cursor.index)));

            ::memcpy(const_cast<char*>(data), cursor.blob.data(), cursor.blob.size());
            return cursor;
        }

        void set_cache_converter(const table_request& request, const cache_converter_interface& converter) {
            auto table = get_table(request);
            cache_.set_cache_converter(table, converter);
        }

        cache_item_ptr create_cache_item(const table_request& req) {
            auto table = get_table(req);
            auto item = cache_.create(table);
            if (BOOST_UNLIKELY(!item)) {
                auto pk = driver_.available_pk(table);
                cache_.set_next_pk(table, pk);
                item = cache_.create(table);
            }
            return item;
        }

        cache_item_ptr get_cache_item(
            const cursor_request& cursor_req, const table_request& table_req, const primary_key_t pk
        ) {
            auto table = get_table(table_req);
            auto item = cache_.find(table, pk);
            if (BOOST_UNLIKELY(!item)) {
                auto obj = object_at_cursor(cursor_req);
                item = cache_.emplace(table, std::move(obj));
            }
            return item;
        }

        const cursor_info& opt_find_by_pk(const table_request& request, primary_key_t pk) {
            auto table = get_table(request);
            return opt_find_by_pk(table, pk);
        }

        // From contracts
        primary_key_t insert(
            apply_context&, const table_request& request, const account_name& payer,
            const primary_key_t pk, const char* data, const size_t size
        ) {
            auto table = get_table(request);
            auto value = table.abi->to_object(table, data, size);
            auto obj = object_value{{table, pk, payer, size}, std::move(value)};

            auto inserted_pk = insert(table, obj);
            // TODO: update RAM usage

            return inserted_pk;
        }

        // From internal
        primary_key_t insert(cache_item& itm, variant value, const size_t size) {
            auto table = get_table(itm);
            auto obj = object_value{{table, itm.pk(), account_name() /* payer */, size}, std::move(value)};

            auto inserted_pk = insert(table, obj);
            itm.set_object(std::move(obj));

            return inserted_pk;
        }

        // From contracts
        primary_key_t update(
            apply_context&, const table_request& request, account_name payer,
            const primary_key_t pk, const char* data, const size_t size
        ) {
            auto table = get_table(request);
            auto value = table.abi->to_object(table, data, size);
            auto obj = object_value{{table, pk, payer, size}, std::move(value)};

            auto updated_pk = update(table, obj);

            // TODO: update RAM usage

            return updated_pk;
        }

        // From internal
        primary_key_t update(cache_item& itm, variant value, const size_t size) {
            auto table = get_table(itm);
            auto obj = object_value{{table, itm.pk(), account_name() /* payer */, size}, std::move(value)};

            auto updated_pk = update(table, obj);
            itm.set_object(std::move(obj));

            return updated_pk;
        }

        // From contracts
        primary_key_t remove(apply_context&, const table_request& request, const primary_key_t pk) {
            auto table = get_table(request);
            auto obj = object_by_pk(table, pk);
            auto removed_pk = remove(table, obj);

            // TODO: update RAM usage

            return removed_pk;
        }

        // From internal
        primary_key_t remove(cache_item& itm) {
            auto table = get_table(itm);
            auto orig_obj = itm.object();
            auto removed_pk = remove(table, std::move(orig_obj));
            cache_.remove(table, removed_pk);

            return removed_pk;
        }

        object_value object_by_pk(const table_request& request, const primary_key_t pk) {
            auto table = get_table(request);
            return object_by_pk(table, pk);
        }

        object_value object_at_cursor(const cursor_request& request) {
            auto& cursor = driver_.current(request);
            auto obj = driver_.object_at_cursor(cursor);
            validate_object(cursor.index, obj, cursor.pk);
            return obj;
        }

    private:
        abi_map abi_map_;

        const cursor_info& opt_find_by_pk(const table_info& table, primary_key_t pk) {
            index_info index(table);
            index.index = &get_pk_index(table);
            auto pk_key_value = _detail::get_pk_value(table, pk);
            return driver_.opt_find_by_pk(index, pk, std::move(pk_key_value));
        }

        table_info get_table(const cache_item& itm) const {
            auto& service = itm.object().service;
            auto info = find_table<index_info>(service);
            CYBERWAY_ASSERT(info.table, unknown_table_exception,
                "ABI table ${table} doesn't exists", ("table", get_full_table_name(service)));

            return std::move(info);
        }

        table_info get_table(const table_request& request) const {
            auto info = find_table<index_info>(request);
            CYBERWAY_ASSERT(info.table, unknown_table_exception,
                "ABI table ${code}.${table} doesn't exists",
                ("code", get_code_name(request))("table", get_table_name(request.hash)));

            return std::move(info);
        }

        index_info get_index(const index_request& request) const {
            auto info = find_index(request);
            CYBERWAY_ASSERT(info.index, unknown_index_exception,
                "ABI index ${code}.${table}.${index} doesn't exists",
                ("code", get_code_name(request))("table", get_table_name(request.hash))
                ("index", get_index_name(request.index)));

            return info;
        }

        template <typename Info, typename Request>
        Info find_table(const Request& request) const {
            Info info(request.code, request.scope);

            auto itr = abi_map_.find(request.code);
            if (abi_map_.end() == itr) return info;

            info.abi = &itr->second;
            info.table = itr->second.find_table(request.hash);
            if (info.table) info.pk_order = &get_pk_order(info);
            return info;
        }

        index_info find_index(const index_request& request) const {
            auto info = find_table<index_info>(request);
            if (info.table == nullptr) return info;

            for (auto& i: info.table->indexes) {
                if (i.hash == request.index) {
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

            auto& obj = driver_.object_at_cursor(cursor);
            validate_object(cursor.index, obj, cursor.pk);

            auto buffer = cursor.index.abi->to_bytes(cursor.index, obj.value);
            driver_.set_blob(cursor, std::move(buffer));
        }

        object_value object_by_pk(const table_info& table, const primary_key_t pk) {
            auto itm = cache_.find(table, pk);
            if (itm) return itm->object();

            auto obj = driver_.object_by_pk(table, pk);
            validate_object(table, obj, pk);
            cache_.emplace(table, obj);
            return obj;
        }

        void validate_pk_field(const table_info& table, const variant& row, const primary_key_t pk) const { try {
            CYBERWAY_ASSERT(pk != unset_primary_key && pk != end_primary_key, primary_key_exception,
                "Value ${pk} can't be used as primary key in the row ${row} "
                "from the table ${table} for the scope '${scope}'",
                ("pk", pk)("row", row)("table", get_full_table_name(table))("scope", get_scope_name(table)));

            auto& pk_order = *table.pk_order;
            const variant* value = &row;
            for (auto& key: pk_order.path) {
                CYBERWAY_ASSERT(value->is_object(), primary_key_exception,
                    "The part ${key} in the primary key is not an object in the row ${row}"
                    "from the table ${table} for the scope '${scope}'",
                    ("key", key)("row", row)("table", get_full_table_name(table))("scope", get_scope_name(table)));

                auto& object = value->get_object();
                auto itr = object.find(key);
                CYBERWAY_ASSERT(object.end() != itr, primary_key_exception,
                    "Can't find the part ${key} for the primary key in the row ${row} "
                    "from the table ${table} for the scope '${scope}'",
                    ("key", key)("row", row)("table", get_full_table_name(table))("scope", get_scope_name(table)));

                value = &(*itr).value();
            }

            auto type = value->get_type();

            auto validate_type = [&](const bool test) {
                CYBERWAY_ASSERT(test, primary_key_exception,
                    "Wrong value type '${type} for the primary key in the row ${row}"
                    "from the table ${table} for the scope '${scope}'",
                    ("type", type)("row", row)("table", get_full_table_name(table))("scope", get_scope_name(table)));
            };

            auto validate_value = [&](const bool test) {
                CYBERWAY_ASSERT(test, primary_key_exception,
                    "Wrong value ${pk} != ${value} for the primary key in the row ${row}"
                    "from the table ${table} for the scope '${scope}'",
                    ("pk", pk)("value", *value)
                    ("type", type)("row", row)("table", get_full_table_name(table))("scope", get_scope_name(table)));
            };

            switch(pk_order.type.front()) {
                case 'i': { // int64
                    validate_type(variant::type_id::int64_type == type);
                    validate_value(value->as_uint64() == pk);
                    break;
                }
                case 'u': { // uint64
                    validate_type(variant::type_id::uint64_type == type);
                    validate_value(value->as_uint64() == pk);
                    break;
                }
                case 'n': { // name
                    validate_type(variant::type_id::string_type == type);
                    validate_value(name(value->as_string()).value == pk);
                    break;
                }
                case 's': { // symbol_code
                    validate_type(variant::type_id::string_type == type);
                    validate_value(symbol(0, value->as_string().c_str()).to_symbol_code() == pk);
                    break;
                }
                default:
                    validate_type(false);
            }
        } catch (const primary_key_exception&) {
            throw;
        } catch (...) {
            CYBERWAY_ASSERT(false, primary_key_exception,
                "Wrong value of the primary key in the row ${row} "
                "from the table ${table} for the scope '${scope}'",
                ("row", row)("table", get_full_table_name(table))("scope", get_scope_name(table)));
        } }

        void validate_object(const table_info& table, const object_value& obj, const primary_key_t pk) const {
            CYBERWAY_ASSERT(obj.value.get_type() == variant::type_id::object_type, invalid_abi_store_type_exception,
                "Receives ${obj} instead of object.", ("obj", obj.value));
            auto& value = obj.value.get_object();

            if (pk == end_primary_key && obj.service.pk == pk) return;

            CYBERWAY_ASSERT(value.end() == value.find(names::service_field), reserved_field_exception,
                "Object has the reserved field ${field} for the table ${table} for the scope '${scope}'",
                ("field", names::service_field)("table", get_full_table_name(table))("scope", get_scope_name(table)));

            validate_pk_field(table, obj.value, pk);
        }

        void set_abi(const account_name& code, abi_info info) {
            info.verify_tables_structure(driver_);
            abi_map_.erase(code);
            abi_map_.emplace(code, std::move(info));
        }

        primary_key_t insert(const table_info& table, object_value& obj) {
            validate_object(table, obj, obj.pk());
            undo_.insert(table, obj);
            return obj.pk();
        }

        primary_key_t update(const table_info& table, object_value& obj) {
            validate_object(table, obj, obj.pk());

            auto orig_obj = object_by_pk(table, obj.pk());
            if (obj.service.payer.empty()) {
                obj.service.payer = orig_obj.service.payer;
            }

            undo_.update(table, std::move(orig_obj), obj);

            return obj.pk();
        }

        primary_key_t remove(const table_info& table, object_value orig_obj) {
            auto pk = orig_obj.pk();
            undo_.remove(table, std::move(orig_obj));
            return pk;
        }
    }; // class chaindb_controller::controller_impl_

    chaindb_controller::chaindb_controller(const chaindb_type t, string p)
    : impl_(new controller_impl_(*this, t, std::move(p))) {
    }

    chaindb_controller::~chaindb_controller() = default;

    void chaindb_controller::restore_db() {
        impl_->restore_db();
    }

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

    const abi_map& chaindb_controller::get_abi_map() const {
        return impl_->get_abi_map();
    }

    chaindb_session chaindb_controller::start_undo_session(bool enabled) {
        return impl_->undo_.start_undo_session(enabled);
    }

    void chaindb_controller::undo() {
        return impl_->undo_.undo();
    }

    void chaindb_controller::undo_all() {
        return impl_->undo_.undo_all();
    }

    void chaindb_controller::commit(const int64_t revision) {
        return impl_->undo_.commit(revision);
    }

    int64_t chaindb_controller::revision() const {
        return impl_->undo_.revision();
    }
    void chaindb_controller::set_revision(uint64_t revision) {
        return impl_->undo_.set_revision(revision);
    }

    void chaindb_controller::close(const cursor_request& request) {
        impl_->driver_.close(request);
    }

    void chaindb_controller::close_code_cursors(const account_name& code) {
        impl_->driver_.close_code_cursors(code);
    }

    void chaindb_controller::apply_all_changes() {
        impl_->driver_.apply_all_changes();
    }

    void chaindb_controller::apply_code_changes(const account_name& code) {
        impl_->driver_.apply_code_changes(code);
    }

    find_info chaindb_controller::lower_bound(const index_request& request, const char* key, size_t size) {
        const auto& info = impl_->lower_bound(request, key, size);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::upper_bound(const index_request& request, const char* key, size_t size) {
        const auto& info = impl_->upper_bound(request, key, size);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::find(const index_request& request, primary_key_t pk, const char* key, size_t size) {
        const auto& info = impl_->find(request, pk, key, size);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::begin(const index_request& request) {
        const auto& info = impl_->begin(request);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::end(const index_request& request) {
        const auto& info = impl_->end(request);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::clone(const cursor_request& request) {
        const auto& info = impl_->driver_.clone(request);
        return {info.id, info.pk};
    }

    primary_key_t chaindb_controller::current(const cursor_request& request) {
        return impl_->driver_.current(request).pk;
    }

    primary_key_t chaindb_controller::next(const cursor_request& request) {
        return impl_->driver_.next(request).pk;
    }

    primary_key_t chaindb_controller::prev(const cursor_request& request) {
        return impl_->driver_.prev(request).pk;
    }

    int32_t chaindb_controller::datasize(const cursor_request& request) {
        return impl_->datasize(request);
    }

    primary_key_t chaindb_controller::data(const cursor_request& request, const char* data, size_t size) {
        return impl_->data(request, data, size).pk;
    }

    void chaindb_controller::set_cache_converter(const table_request& table, const cache_converter_interface& conv) {
        impl_->set_cache_converter(table, conv);
    }

    cache_item_ptr chaindb_controller::create_cache_item(const table_request& table) {
        return impl_->create_cache_item(table);
    }

    cache_item_ptr chaindb_controller::get_cache_item(
        const cursor_request& cursor, const table_request& table, const primary_key_t pk
    ) {
        return impl_->get_cache_item(cursor, table, pk);
    }

    primary_key_t chaindb_controller::available_pk(const table_request& request) {
        return impl_->available_pk(request);
    }

    primary_key_t chaindb_controller::insert(
        apply_context& ctx, const table_request& request, const account_name& payer,
        primary_key_t pk, const char* data, size_t size
    ) {
         return impl_->insert(ctx, request, payer, pk, data, size);
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

    primary_key_t chaindb_controller::insert(cache_item& itm, variant data, size_t size) {
        return impl_->insert(itm, std::move(data), size);
    }

    primary_key_t chaindb_controller::update(cache_item& itm, variant data, size_t size
    ) {
        return impl_->update(itm, std::move(data), size);
    }

    primary_key_t chaindb_controller::remove(cache_item& itm) {
        return impl_->remove(itm);
    }

    variant chaindb_controller::value_by_pk(const table_request& request, primary_key_t pk) {
        return impl_->object_by_pk(request, pk).value;
    }

    variant chaindb_controller::value_at_cursor(const cursor_request& request) {
        return impl_->object_at_cursor(request).value;
    }

    find_info chaindb_controller::opt_find_by_pk(const table_request& request, primary_key_t pk) {
        const auto& info = impl_->opt_find_by_pk(request, pk);
        return {info.id, info.pk};
    }
} } // namespace cyberway::chaindb
