#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/object_value.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/abi_info.hpp>
#include <cyberway/chaindb/storage_calculator.hpp>
#include <cyberway/chaindb/storage_payer_info.hpp>

#include <eosio/chain/name.hpp>
#include <eosio/chain/symbol.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/transaction_context.hpp>

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
        std::unique_ptr<driver_interface> create_driver(
            chaindb_type type, journal& jrnl, string address, string sys_name
        ) {
            if (sys_name.empty()) {
                sys_name = names::system_code;
            }

            switch(type) {
                case chaindb_type::MongoDB:
                    return std::make_unique<mongodb_driver>(jrnl, std::move(address), std::move(sys_name));

                default:
                    break;
            }
            CYBERWAY_ASSERT(
                false, unknown_connection_type_exception,
                "Invalid type ${type} of ChainDB connection", ("type", type));
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

    //------------------------------------

    void storage_payer_info::calc_usage(const table_info& table, const object_value& obj) {
        // TODO: is it a crutch?
        if (BOOST_UNLIKELY(payer == eosio::chain::config::system_account_name)) {
            size  = 0;
            delta = 0;
        } else if (BOOST_UNLIKELY(payer.empty())) {
            size  = 0;
            delta = 0;
        } else if (!size) {
            size  = chaindb::calc_storage_usage(*table.table, obj.value);
            delta = 0;
        }
    }

    void storage_payer_info::add_usage() {
        // TODO: is it a crutch?
        if (BOOST_UNLIKELY(payer == eosio::chain::config::system_account_name)) {
            // do nothing
        } else if (BOOST_UNLIKELY(payer.empty() || (!delta && !size))) {
            // do nothing
        } else if (BOOST_LIKELY(!!apply_ctx)) {
            apply_ctx->add_storage_usage(*this);
        } else if (!!transaction_ctx) {
            transaction_ctx->add_storage_usage(*this);
        } else if (!!resource_mng) {
            resource_mng->add_storage_usage(*this);
        }
    }

    void storage_payer_info::get_payer_from(const object_value& obj) {
        owner  = obj.service.owner;
        payer  = obj.service.payer;
        size   = obj.service.size;
        in_ram = obj.service.in_ram;
    }

    void storage_payer_info::set_payer_in(object_value& obj) const {
        obj.service.owner  = owner;
        obj.service.payer  = payer;
        obj.service.size   = size;
        obj.service.in_ram = in_ram;
    }

    //------------------------------------

    struct chaindb_controller::controller_impl_ final {
        journal journal_;
        std::unique_ptr<driver_interface> driver_ptr_;
        driver_interface& driver_;
        abi_map abi_map_;
        cache_map cache_;
        undo_stack undo_;

        controller_impl_(chaindb_controller& controller, const chaindb_type t, string address, string sys_name)
        : driver_ptr_(_detail::create_driver(t, journal_, std::move(address), std::move(sys_name))),
          driver_(*driver_ptr_.get()),
          cache_(abi_map_),
          undo_(controller, driver_, journal_, cache_) {
        }

        ~controller_impl_() = default;

        void restore_db() {
            undo_.restore();
        }

        void drop_db() {
            const auto itr = abi_map_.find(get_system_code());

            assert(itr != abi_map_.end());

            auto system_abi = std::move(itr->second);
            cache_.clear(); // reset all cached values
            undo_.clear(); // remove all undo states
            journal_.clear(); // remove all pending changes
            driver_.drop_db(); // drop database
            abi_map_.clear(); // clear ABIes
            set_abi(get_system_code(), std::move(system_abi));
        }

        void set_abi(const account_name& code, abi_def abi) {
            if (is_system_code(code)) undo_.add_abi_tables(abi);

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

        const cursor_info& current(const cursor_info& cursor) const {
            if (unset_primary_key == cursor.pk) {
                driver_.current(cursor);
            }
            return cursor;
        }

        const cursor_info& current(const cursor_request& request) const {
            return current(driver_.cursor(request));
        }

        const cursor_info& lower_bound(const index_request& request, const char* key, const size_t size) const {
            auto  index  = get_index(request);
            auto  value  = index.abi->to_object(index, key, size);
            auto& cursor = driver_.lower_bound(std::move(index), std::move(value));

            if (index.index->unique) {
                auto cache = cache_.find(index, key, size);
                if (cache) {
                    cursor.pk = cache->pk();
                    return cursor;
                }
            }

            return current(cursor);
        }

        const cursor_info& lower_bound(const table_request& request, const primary_key_t pk) const {
            auto  index  = get_pk_index(request);
            auto  value  = _detail::get_pk_value(index, pk);
            auto  cache  = cache_.find(index, pk);
            auto& cursor = driver_.lower_bound(std::move(index), std::move(value));

            if (cache) {
                cursor.pk = pk;
            } else {
                current(cursor);
            }
            return cursor;
        }

        // API request, it can't use cache
        const cursor_info& lower_bound(const index_request& request, const variant& orders) const {
            return current(driver_.lower_bound(get_index(request), orders));
        }

        const cursor_info& upper_bound(const index_request& request, const char* key, const size_t size) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size);
            return current(driver_.upper_bound(std::move(index), std::move(value)));
        }

        const cursor_info& upper_bound(const table_request& request, const primary_key_t pk) const {
            auto index = get_pk_index(request);
            auto value = _detail::get_pk_value(index, pk);
            return current(driver_.upper_bound(std::move(index), std::move(value)));
        }

        const cursor_info& upper_bound(const index_request& request, const variant& orders) const {
            return current(driver_.upper_bound(get_index(request), orders));
        }

        const cursor_info& locate_to(const index_request& request, const char* key, size_t size, primary_key_t pk) const {
            auto index = get_index(request);
            auto value = index.abi->to_object(index, key, size);
            return driver_.locate_to(std::move(index), std::move(value), pk);
        }

        const cursor_info& begin(const index_request& request) const {
            return current(driver_.begin(get_index(request)));
        }

        const cursor_info& end(const index_request& request) const {
            return driver_.end(get_index(request));
        }

        primary_key_t available_pk(const table_request& request) const {
            return driver_.available_pk(get_table(request));
        }

        void set_cache_converter(const table_request& request, const cache_converter_interface& converter) {
            auto table = get_table(request);
            cache_.set_cache_converter(table, converter);
        }

        cache_object_ptr create_cache_object(const table_request& req) {
            auto table = get_table(req);
            auto item = cache_.create(table);
            if (BOOST_UNLIKELY(!item)) {
                auto pk = driver_.available_pk(table);
                cache_.set_next_pk(table, pk);
                item = cache_.create(table);
            }
            return item;
        }

        cache_object_ptr get_cache_object(const cursor_request& req, const bool with_blob) {
            auto& cursor = current(req);

            CYBERWAY_ASSERT(end_primary_key != cursor.pk, driver_absent_object_exception,
                "Requesting object from the end of the table ${table}",
                ("table", get_full_table_name(cursor.index)));

            auto item = cache_.find(cursor.index, cursor.pk);
            if (BOOST_UNLIKELY(!item)) {
                auto obj = object_at_cursor(cursor);
                if (!obj.is_null()) {
                    item = cache_.emplace(cursor.index, std::move(obj));
                }
            }

            if (item && with_blob && !item->has_blob()) {
                auto& table  = static_cast<const table_info&>(cursor.index);
                auto  buffer = cursor.index.abi->to_bytes(table, item->object().value); // 1 Mb
                item->set_blob(bytes(buffer.begin(), buffer.end()));                    // Minimize memory usage
            }

            return item;
        }

        // From contracts
        int insert(
            const table_request& request, const storage_payer_info& storage,
            const primary_key_t pk, const char* data, const size_t size
        ) {
            auto table = get_table(request);
            auto value = table.abi->to_object(table, data, size);
            auto obj = object_value{{table, pk}, std::move(value)};

            return insert(table, storage, obj);
        }

        // From internal
        int insert(cache_object& itm, variant value, const storage_payer_info& storage) {
            auto table = get_table(itm);
            auto obj = object_value{{table, itm.pk()}, std::move(value)};

            auto delta = insert(table, storage, obj);
            itm.set_object(std::move(obj));

            return delta;
        }

        // From genesis
        int insert(const table_request& request, primary_key_t pk, variant value, const storage_payer_info& storage) {
            auto table = get_table(request);
            auto obj = object_value{{table, pk}, std::move(value)};
            return insert(table, storage, obj);
        }

        // From contracts
        int update(
            const table_request& request, storage_payer_info storage,
            const primary_key_t pk, const char* data, const size_t size
        ) {
            auto table = get_table(request);
            auto value = table.abi->to_object(table, data, size);
            auto obj = object_value{{table, pk}, std::move(value)};
            auto orig_obj = object_by_pk(table, obj.pk());

            storage.in_ram = orig_obj.service.in_ram;
            auto delta = update(table, std::move(storage), obj, std::move(orig_obj));
            cache_.emplace(table, std::move(obj));

            return delta;
        }

        // From internal
        int update(cache_object& itm, variant value, storage_payer_info storage) {
            auto table = get_table(itm);
            auto obj = object_value{{table, itm.pk()}, std::move(value)};

            storage.in_ram = itm.service().in_ram;
            auto delta = update(table, std::move(storage), obj, itm.object());
            itm.set_object(std::move(obj));

            return delta;
        }

        void recalc_ram_usage(cache_object& itm, storage_payer_info storage) {
            auto table = get_table(itm);
            auto obj = itm.object();
            auto orig_obj = object_by_pk(table, obj.pk());

            // obj.service.in_ram = ram_payer.in_ram;
            storage.size  = orig_obj.service.size;
            storage.delta = 0;
            update(table, storage, obj, std::move(orig_obj));
            itm.set_service(std::move(obj.service));
        }

        // From contracts
        int remove(const table_request& request, const storage_payer_info& storage, const primary_key_t pk) {
            auto table = get_table(request);
            auto obj = object_by_pk(table, pk);

            return remove(table, storage, obj);
        }

        // From internal
        int remove(cache_object& itm, const storage_payer_info& storage) {
            auto table = get_table(itm);
            auto orig_obj = itm.object();

            return remove(table, storage, std::move(orig_obj));
        }

        object_value object_by_pk(const table_request& request, const primary_key_t pk) {
            auto table = get_table(request);
            return object_by_pk(table, pk);
        }

        object_value object_at_cursor(const cursor_request& request) {
            return object_at_cursor(current(request));
        }

    private:
        object_value object_at_cursor(const cursor_info& cursor) {
            auto obj = driver_.object_at_cursor(cursor);
            validate_object(cursor.index, obj, cursor.pk);
            return obj;
        }

        table_info get_table(const cache_object& itm) const {
            auto& service = itm.object().service;
            auto info = find_table<table_info>(service);
            CYBERWAY_ASSERT(info.table, unknown_table_exception,
                "ABI table ${table} doesn't exists", ("table", get_full_table_name(service)));

            return info;
        }

        template <typename Request>
        table_info get_table(const Request& request) const {
            auto info = find_table<table_info>(request);
            CYBERWAY_ASSERT(info.table, unknown_table_exception,
                "ABI table ${code}.${table} doesn't exists",
                ("code", get_code_name(request))("table", get_table_name(request.table)));

            return info;
        }

        index_info get_index(const index_request& request) const {
            auto info = find_index(request);
            CYBERWAY_ASSERT(info.index, unknown_index_exception,
                "ABI index ${code}.${table}.${index} doesn't exists",
                ("code", get_code_name(request))("table", get_table_name(request.table))
                ("index", get_index_name(request.index)));

            return info;
        }

        template <typename Request>
        index_info get_pk_index(const Request& request) const {
            auto table = get_table(request);
            auto index = index_info(table);
            index.index = &chaindb::get_pk_index(table);
            return index;
        }

        template <typename Info, typename Request>
        Info find_table(const Request& request) const {
            Info info(request.code, request.scope);

            auto itr = abi_map_.find(request.code);
            if (abi_map_.end() == itr) return info;

            info.abi = &itr->second;
            info.table = itr->second.find_table(request.table);
            if (info.table) info.pk_order = &get_pk_order(info);
            return info;
        }

        index_info find_index(const index_request& request) const {
            auto info = find_table<index_info>(request);
            if (info.table == nullptr) return info;

            for (auto& i: info.table->indexes) {
                if (i.name == request.index) {
                    info.index = &i;
                    break;
                }
            }
            return info;
        }

        object_value object_by_pk(const table_info& table, const primary_key_t pk) {
            auto itm = cache_.find(table, pk);
            if (itm) return itm->object();

            auto obj = driver_.object_by_pk(table, pk);
            validate_object(table, obj, pk);
            if (!obj.value.is_null()) {
                cache_.emplace(table, obj);
            }
            return obj;
        }

        void validate_pk_field(const table_info& table, const variant& row, const primary_key_t pk) const try {
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
        }

        void validate_object(const table_info& table, const object_value& obj, const primary_key_t pk) const {
            if (end_primary_key == obj.pk()) {
                CYBERWAY_ASSERT(obj.is_null(), driver_wrong_object_exception,
                    "Driver returns the row '${obj}' from the table ${table} instead of null for end iterator",
                    ("obj", obj.value)("table", get_full_table_name(table)));
                return;
            }

            CYBERWAY_ASSERT(obj.value.is_object(), invalid_abi_store_type_exception,
                "Receives ${obj} instead of object from the table ${table}",
                ("obj", obj.value)("table", get_full_table_name(table)));
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

        int insert(const table_info& table, storage_payer_info charge, object_value& obj) {
            validate_object(table, obj, obj.pk());

            charge.calc_usage(table, obj);
            charge.in_ram = true;
            charge.delta  = charge.size;

            // charge the payer
            charge.add_usage();

            // insert object to storage
            charge.set_payer_in(obj);
            obj.service.revision = undo_.revision();

            undo_.insert(table, obj);

            return charge.delta;
        }

        int update(const table_info& table, storage_payer_info charge, object_value& obj, object_value orig_obj) {
            validate_object(table, obj, obj.pk());

            if (charge.owner.empty()) {
                charge.owner = orig_obj.service.owner;
            }

            if (charge.payer.empty()) {
                charge.payer = orig_obj.service.payer;
            }

            charge.calc_usage(table, obj);
            auto delta = charge.size - orig_obj.service.size;

            if (charge.payer  != orig_obj.service.payer ||
                charge.owner  != orig_obj.service.owner ||
                charge.in_ram != orig_obj.service.in_ram
            ) {
                auto refund = charge;
                refund.get_payer_from(orig_obj);
                refund.delta = -orig_obj.service.size;
                // refund the existing payer
                refund.add_usage();

                charge.delta = charge.size;
            } else {
                charge.delta = delta;
            }

            // charge the new payer
            charge.add_usage();

            // update object in storage
            charge.set_payer_in(obj);
            obj.service.revision = undo_.revision();

            undo_.update(table, std::move(orig_obj), obj);

            return delta;
        }

        int remove(const table_info& table, storage_payer_info refund, object_value orig_obj) {
            auto pk = orig_obj.pk();

            refund.get_payer_from(orig_obj);
            refund.delta  = -orig_obj.service.size;

            // refund the payer
            refund.add_usage();

            undo_.remove(table, std::move(orig_obj));
            cache_.remove(table, pk);

            return refund.delta;
        }
    }; // class chaindb_controller::controller_impl_

    chaindb_controller::chaindb_controller(const chaindb_type t, string address, string sys_name)
    : impl_(new controller_impl_(*this, t, std::move(address), std::move(sys_name))) {
    }

    chaindb_controller::~chaindb_controller() = default;

    void chaindb_controller::restore_db() {
        impl_->restore_db();
    }

    void chaindb_controller::drop_db() {
        impl_->drop_db();
    }

    void chaindb_controller::clear_cache() {
        impl_->cache_.clear();
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

    find_info chaindb_controller::lower_bound(const index_request& request, const char* key, size_t size) const {
        const auto& info = impl_->lower_bound(request, key, size);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::lower_bound(const table_request& request, const primary_key_t pk) const {
        const auto& info = impl_->lower_bound(request, pk);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::lower_bound(const index_request& request, const variant& orders) const {
        auto info = impl_->lower_bound(request, orders);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::upper_bound(const index_request& request, const char* key, size_t size) const {
        const auto& info = impl_->upper_bound(request, key, size);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::upper_bound(const table_request& request, const primary_key_t pk) const {
        const auto& info = impl_->upper_bound(request, pk);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::upper_bound(const index_request& request, const variant& orders) const {
        auto info = impl_->lower_bound(request, orders);
        return {info.id, info.pk};
    }

    find_info chaindb_controller::locate_to(const index_request& request, const char* key, size_t size, primary_key_t pk) {
        const auto& info = impl_->locate_to(request, key, size, pk);
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
        return impl_->current(request).pk;
    }

    primary_key_t chaindb_controller::next(const cursor_request& request) {
        auto& driver = impl_->driver_;
        return driver.next(driver.cursor(request)).pk;
    }

    primary_key_t chaindb_controller::prev(const cursor_request& request) {
        auto& driver = impl_->driver_;
        return driver.prev(driver.cursor(request)).pk;
    }

    void chaindb_controller::set_cache_converter(const table_request& table, const cache_converter_interface& conv) {
        impl_->set_cache_converter(table, conv);
    }

    cache_object_ptr chaindb_controller::create_cache_object(const table_request& table) {
        return impl_->create_cache_object(table);
    }

    cache_object_ptr chaindb_controller::get_cache_object(const cursor_request& cursor, const bool with_blob) {
        return impl_->get_cache_object(cursor, with_blob);
    }

    primary_key_t chaindb_controller::available_pk(const table_request& request) {
        return impl_->available_pk(request);
    }

    int chaindb_controller::insert(
        const table_request& request, const storage_payer_info& storage,
        primary_key_t pk, const char* data, size_t size
    ) {
         return impl_->insert(request, storage, pk, data, size);
    }

    int chaindb_controller::update(
        const table_request& request, const storage_payer_info& storage,
        primary_key_t pk, const char* data, size_t size
    ) {
        return impl_->update(request, storage, pk, data, size);
    }

    int chaindb_controller::remove(const table_request& request, const storage_payer_info& storage, primary_key_t pk) {
        return impl_->remove(request, storage, pk);
    }

    int chaindb_controller::insert(
        const table_request& request, primary_key_t pk, variant data, const storage_payer_info& storage
    ) {
         return impl_->insert(request, pk, std::move(data), storage);
    }

    int chaindb_controller::insert(cache_object& itm, variant data, const storage_payer_info& storage) {
        return impl_->insert(itm, std::move(data), storage);
    }

    int chaindb_controller::update(cache_object& itm, variant data, const storage_payer_info& storage) {
        return impl_->update(itm, std::move(data), storage);
    }

    int chaindb_controller::remove(cache_object& itm, const storage_payer_info& storage) {
        return impl_->remove(itm, storage);
    }

    void chaindb_controller::recalc_ram_usage(cache_object& itm, const storage_payer_info& storage) {
        impl_->recalc_ram_usage(itm, storage);
    }

    variant chaindb_controller::value_by_pk(const table_request& request, primary_key_t pk) {
        return impl_->object_by_pk(request, pk).value;
    }

    variant chaindb_controller::value_at_cursor(const cursor_request& request) {
        return impl_->object_at_cursor(request).value;
    }

    object_value chaindb_controller::object_at_cursor(const cursor_request& request) const {
        return impl_->object_at_cursor(request);
    }

} } // namespace cyberway::chaindb
