#include <limits>

#include <cyberway/chaindb/mongo_big_int_converter.hpp>
#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/table_object.hpp>
#include <cyberway/chaindb/mongo_driver_utils.hpp>

#include <eosio/chain/symbol.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/query_exception.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::name;
    using eosio::chain::symbol;

    using fc::optional;
    using fc::variant_object;

    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::document;
    using bsoncxx::builder::basic::sub_document;
    using bsoncxx::builder::basic::kvp;

    using mongocxx::bulk_write;
    using mongocxx::model::insert_one;
    using mongocxx::model::update_one;
    using mongocxx::model::delete_one;
    using mongocxx::database;
    using mongocxx::collection;
    using mongocxx::query_exception;
    namespace options = mongocxx::options;

    using document_view = bsoncxx::document::view;

    enum class direction: int {
        Forward = 1,
        Backward = -1
    }; // enum direction

    struct cmp_info {
        const direction order;
        const char* from_forward;
        const char* from_backward;
        const char* to_forward;
        const char* to_backward;
    }; // struct cmp_info

    enum class mongo_code: int {
        Unknown = -1,
        Duplicate = 11000,
        NoServer = 13053
    };

    namespace { namespace _detail {

        template <typename Exception>
        mongo_code get_mongo_code(const Exception& e) {
            auto value = static_cast<mongo_code>(e.code().value());

            switch (value) {
                case mongo_code::Duplicate:
                case mongo_code::NoServer:
                    return value;

                default:
                    return mongo_code::Unknown;
            }
        }

        const cmp_info& start_from() {
            static cmp_info value = {
                direction::Forward,
                "$gte", "$lte",
                "$gt",  "$lt" };
            return value;
        }

        const cmp_info& start_after() {
            static cmp_info value = {
                direction::Forward,
                "$gt", "$lt",
                nullptr, nullptr };
            return value;
        }

        const cmp_info& reverse_start_from() {
            static cmp_info value = {
                direction::Backward,
                "$lte", "$gte",
                "$lt",  "$gt" };
            return value;
        }

        bool is_asc_order(const string& name) {
            return name.size() == 3; // asc (vs desc)
        }

        variant get_order_value(variant_object object, const index_info& index, const order_def& order) { try {
            auto pos = order.path.size();
            for (auto& key: order.path) {
                auto itr = object.find(key);
                CYBERWAY_ASSERT(object.end() != itr, driver_absent_field_exception, "Wrong path");

                --pos;
                if (0 == pos) {
                    return itr->value();
                } else {
                    object = itr->value().get_object();
                }
            }
            CYBERWAY_ASSERT(false, driver_absent_field_exception, "Invalid type of primary key");
        } catch (...) {
            CYBERWAY_ASSERT(false, driver_absent_field_exception,
                "External database returns the row in the table ${table} without the field ${field}.",
                ("table", get_full_table_name(index))("field", order.field));
        } }

        primary_key_t get_pk_value(const table_info& table, document_view view) { try {
            auto& pk_order = *table.pk_order;
            auto pos = pk_order.path.size();
            for (auto& key: pk_order.path) {
                auto itr = view.find(key);
                CYBERWAY_ASSERT(view.end() != itr, driver_primary_key_exception, "Wrong path");

                --pos;
                if (0 == pos) {
                    switch (pk_order.type.front()) {
                        case 'i': // int64
                            return static_cast<primary_key_t>(itr->get_int64().value);
                        case 'u': { // uint64
                            auto as_string = itr->get_decimal128().value.to_string();
                            return static_cast<primary_key_t>(std::stoull(as_string));
                        }
                        case 'n': // name
                            return name(itr->get_utf8().value.data()).value;
                        case 's': // symbol_code
                            return symbol(0, itr->get_utf8().value.data()).to_symbol_code();
                        default:
                            break;
                    }
                } else {
                    view = itr->get_document().value;
                }
            }
            CYBERWAY_ASSERT(false, driver_primary_key_exception, "Invalid type of primary key");
        } catch (...) {
            CYBERWAY_ASSERT(false, driver_primary_key_exception,
                "External database locate row in the table ${table} with wrong value of primary key.",
                ("table", get_full_table_name(table)));
        } }

        document make_pk_document(const table_info& table, primary_key_t pk) {
            document doc;
            doc.append(kvp(get_scope_field_name(), get_scope_name(table)));

            auto& pk_order = *table.pk_order;
            switch (pk_order.type.front()) {
                case 'i': // int64
                    doc.append(kvp(pk_order.field, static_cast<int64_t>(pk)));
                    break;
                case 'u': // uint64
                    doc.append(kvp(pk_order.field, to_decimal128(pk)));
                    break;
                case 'n': // name
                    doc.append(kvp(pk_order.field, name(pk).to_string()));
                    break;
                case 's': // symbol_code
                    doc.append(kvp(pk_order.field, symbol(pk << 8).name()));
                    break;
                default:
                    CYBERWAY_ASSERT(false, driver_primary_key_exception, "Invalid type of primary key");
            }

            return doc;
        }

        template <typename Lambda>
        void auto_reconnect(Lambda&& lambda) {
            // TODO: move to program options
            constexpr int max_iters = 12;
            constexpr int sleep_seconds = 5;

            for (int i = 0; i < max_iters; ++i) { try {
                if (i > 0) {
                    wlog("Fail to connect to MongoDB server, wait ${sec} seconds...", ("sec", sleep_seconds));
                    sleep(sleep_seconds);
                    wlog("Try again...");
                }
                lambda();
                return;
            } catch (const mongocxx::exception& e) {
                if (_detail::get_mongo_code(e) == mongo_code::NoServer) {
                    continue; // try again
                }

                throw;
            } }

            CYBERWAY_ASSERT(false, driver_open_exception, "Fail to connect to MongoDB server");
        }

    } } // namespace _detail

    class mongodb_cursor_info: public cursor_info {
    public:
        mongodb_cursor_info(cursor_t id, index_info index, collection db_table)
        : cursor_info{id, std::move(index)},
          db_table_(std::move(db_table)) {
        }

        mongodb_cursor_info() = default;
        mongodb_cursor_info(mongodb_cursor_info&&) = default;

        mongodb_cursor_info(const mongodb_cursor_info&) = delete;

        mongodb_cursor_info clone(cursor_t id) {
            mongodb_cursor_info dst(id, index, db_table_);
            dst.pk = pk;
            dst.blob = blob;

            if (find_cmp_->order == direction::Forward) {
                dst.find_cmp_ = &_detail::start_from();
            } else {
                dst.find_cmp_ = &_detail::reverse_start_from();
            }

            if (source_.valid()) {
                // it is faster to get object from exist cursor then to open a new cursor, locate, and get object
                dst.object_ = get_object_value();
                dst.find_key_ = dst.object_;
                dst.find_pk_ = get_pk_value();
            } else {
                dst.find_key_ = find_key_;
                dst.find_pk_ = find_pk_;
                dst.object_ = object_;
            }

            return dst;
        }

        void open(const cmp_info& find_cmp, variant key, const primary_key_t locate_pk = unset_primary_key) {
            pk = unset_primary_key;
            source_.reset();
            reset();

            find_cmp_ = &find_cmp;
            find_key_ = std::move(key);
            find_pk_ = locate_pk;
        }

        void open_by_pk(const cmp_info& find_cmp, variant key, const primary_key_t locate_pk) {
            source_.reset();
            reset();
            pk = locate_pk;

            find_cmp_ = &find_cmp;
            find_key_ = std::move(key);
            find_pk_ = unset_primary_key;
        }

        void open_end() {
            pk = end_primary_key;
            source_.reset();
            reset();
            find_key_ = {};
            find_cmp_ = &_detail::reverse_start_from();
            find_pk_ = unset_primary_key;
        }

        void next() {
            if (find_cmp_->order == direction::Backward) {
                change_direction(_detail::start_from());
            }
            lazy_next();
        }

        void prev() {
            if (find_cmp_->order == direction::Forward) {
                change_direction(_detail::reverse_start_from());
            } else if (end_primary_key == pk) {
                lazy_open();
                return;
            }
            lazy_next();
        }

        void current() {
            lazy_open();
        }

        const variant& get_object_value() {
            lazy_open();
            if (is_end() || !object_.is_null()) return object_;

            auto& view = *source_->begin();
            object_ = build_variant(view);
            pk = _detail::get_pk_value(index, view);

            return object_;
        }

    private:
        collection db_table_;

        const cmp_info* find_cmp_ = &_detail::start_from();
        variant find_key_;
        primary_key_t find_pk_ = unset_primary_key;

        optional<mongocxx::cursor> source_;
        variant object_;

        bool is_end() const {
            return end_primary_key == pk;
        }

        void change_direction(const cmp_info& find_cmp) {
            find_cmp_ = &find_cmp;
            if (source_.valid()) {
                find_key_ = get_object_value();
                find_pk_ = get_pk_value();
            }

            source_.reset();
        }

        void reset() {
            if (!blob.empty()) blob.clear();
            if (!object_.is_null()) object_ = {};
        }

        document create_find_document(const char* forward, const char* backward) const {
            document find;

            find.append(kvp(get_scope_field_name(), get_scope_name(index)));
            if (!find_key_.is_object()) return find;

            auto& find_object = find_key_.get_object();
            if (!find_object.size()) return find;

            auto& orders = index.index->orders;
            for (auto& o: orders) {
                auto cmp = _detail::is_asc_order(o.order) ? forward : backward;
                auto value = _detail::get_order_value(find_object, index, o);
                find.append(kvp(o.field, [&](sub_document doc){build_document(doc, cmp, value);}));
            }

            return find;
        }

        document create_sort_document() const {
            document sort;
            auto order = static_cast<int>(find_cmp_->order);

            auto& orders = index.index->orders;
            for (auto& o: orders) {
                if (_detail::is_asc_order(o.order)) {
                    sort.append(kvp(o.field, order));
                } else {
                    sort.append(kvp(o.field, -order));
                }
            }

            if (!index.index->unique) {
                sort.append(kvp(index.pk_order->field, order));
            }

            return sort;
        }

        void lazy_open() {
            if (source_) return;

            reset();
            pk = unset_primary_key;

            auto find = create_find_document(find_cmp_->from_forward, find_cmp_->from_backward);
            auto sort = create_sort_document();

            const auto find_pk = find_pk_;
            find_pk_ = unset_primary_key;

            _detail::auto_reconnect([&]() {
                source_.emplace(db_table_.find(find.view(), options::find().sort(sort.view())));

                init_pk_value();
                if (unset_primary_key == find_pk || find_pk == pk || end_primary_key == pk) return;
                if (index.index->unique || !find_cmp_->to_forward) return;

                // locate cursor to primary key

                auto to_find = create_find_document(find_cmp_->to_forward, find_cmp_->to_backward);
                auto to_cursor = db_table_.find(to_find.view(), options::find().sort(sort.view()).limit(1));

                auto to_pk = unset_primary_key;
                auto to_itr = to_cursor.begin();
                if (to_itr != to_cursor.end()) to_pk = _detail::get_pk_value(index, *to_itr);

                // TODO: limitation by deadline
                static constexpr int max_iters = 10000;
                auto itr = source_->begin();
                auto etr = source_->end();
                for (int i = 0; i < max_iters && itr != etr; ++i, ++itr) {
                    pk = _detail::get_pk_value(index, *to_itr);
                    // range is end, but pk not found
                    if (to_pk == pk) break;
                    // ok, key is found
                    if (find_pk == pk) return;
                }

                open_end();
            });
        }

        void lazy_next() {
            lazy_open();
            reset();

            auto itr = source_->begin();
            ++itr;
            CYBERWAY_ASSERT(find_cmp_->order != direction::Backward || itr != source_->end(),
                driver_out_of_range_exception,
                "External database tries to locate in the ${table} in out of range",
                ("table", get_full_table_name(index)));
            init_pk_value();
        }

        primary_key_t get_pk_value() {
            if (unset_primary_key == pk) {
                init_pk_value();
            }
            return pk;
        }

        void init_pk_value() {
            auto itr = source_->begin();
            if (source_->end() == itr) {
                open_end();
            } else {
                pk = _detail::get_pk_value(index, *itr);
            }
        }

    }; // class mongodb_cursor_info

    using cursor_map = std::map<cursor_t, mongodb_cursor_info>;
    using code_cursor_map = std::map<account_name /* code */, cursor_map>;

    struct cursor_location {
        code_cursor_map::iterator code_itr_;
        cursor_map::iterator cursor_itr_;

        mongodb_cursor_info& cursor() {
            return cursor_itr_->second;
        }

        cursor_map& map() {
            return code_itr_->second;
        }
    }; // struct cursor_location

    enum class changed_value_state {
        Unknown,
        Insert,
        Update,
        Delete
    }; // enum class value_state

    struct changed_value_info {
        changed_value_state state = changed_value_state::Unknown;
        variant value;
    }; // struct value_info

    using changed_value_map = std::map<primary_key_t, changed_value_info>;

    struct table_changed_object final: public table_object::object {
        using table_object::object::object;
        changed_value_map map;
    }; // struct table_changed_object

    using table_changed_object_index = table_object::index<table_changed_object>;

    struct changed_value_location {
        table_changed_object_index::iterator table_itr_;
        changed_value_map::iterator value_itr_;

        table_changed_object& table() {
            // not critical, because key parts are const
            return const_cast<table_changed_object&>(*table_itr_);
        }

        primary_key_t pk() {
            return value_itr_->first;
        }

        changed_value_info& info() {
            return value_itr_->second;
        }
    }; // struct changed_value_location

    struct mongodb_driver::mongodb_impl_ {
        mongocxx::client mongo_conn_;
        code_cursor_map code_cursor_map_;
        table_changed_object_index table_changed_value_map_;

        mongodb_impl_(const std::string& p) {
            init_instance();
            mongocxx::uri uri{p};
            mongo_conn_ = mongocxx::client{uri};
        }

        ~mongodb_impl_() = default;

        cursor_location get_applied_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            auto& cursor = loc.cursor();

            if (cursor.pk == unset_primary_key) {
                apply_table_changes(cursor.index);
            }
            return loc;
        }

        void apply_code_changes(const account_name& code) {
            auto begin_itr = table_changed_value_map_.lower_bound(std::make_tuple(code));
            auto end_itr = table_changed_value_map_.upper_bound(std::make_tuple(code));

            apply_range_changes(begin_itr, end_itr);
        }

        void apply_all_changes() {
            apply_range_changes(table_changed_value_map_.begin(), table_changed_value_map_.end());
        }

        void close_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            auto& map = loc.map();

            map.erase(loc.cursor_itr_);
            if (map.empty()) {
                code_cursor_map_.erase(loc.code_itr_);
            }
        }

        void close_all_cursors(const account_name& code) {
            auto itr = code_cursor_map_.find(code);
            if (code_cursor_map_.end() == itr) return;

//            CYBERWAY_ASSERT(!itr->second.has_changes(), driver_has_unapplied_changes_exception,
//                "Chaindb has unapplied changes for the db ${db}", ("db", get_code_name(code)));
            code_cursor_map_.erase(itr);
        }

        mongodb_cursor_info& clone_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            auto next_id = get_next_cursor_id(loc.code_itr_);

            auto cloned_cursor = loc.cursor().clone(next_id);
            return add_cursor(loc.code_itr_, request.code, std::move(cloned_cursor));
        }

        void verify_table_structure(const table_info& table, const microseconds& max_time) {
            auto db_table = get_db_table(table);
            auto& indexes = table.table->indexes;

            // TODO: add removing of old indexes

            for (auto& index: indexes) {
                bool was_pk = false;
                document doc;
                doc.append(kvp(get_scope_field_name(), 1));
                for (auto& o: index.orders) {
                    auto field = o.field;
                    // ui128 || i128
                    if (o.type.back() == '8' && (o.type.front() == 'i' || o.type.front() == 'u')) {
                        field.append(".").append(mongo_big_int_converter::BINARY_FIELD);
                    }
                    if (_detail::is_asc_order(o.order)) {
                        doc.append(kvp(field, 1));
                    } else {
                        doc.append(kvp(field, -1));
                    }
                    was_pk |= (&o == table.pk_order);
                }
                if (!was_pk && !index.unique) {
                    doc.append(kvp(table.pk_order->field, 1));
                }

                auto db_index_name = get_index_name(index);
                _detail::auto_reconnect([&]() {
                    try {
                        db_table.create_index(doc.view(), options::index().name(db_index_name).unique(index.unique));
                    } catch (const mongocxx::operation_exception& e) {
                        db_table.indexes().drop_one(db_index_name);
                        db_table.create_index(doc.view(), options::index().name(db_index_name).unique(index.unique));
                    }
                });
            }
        }

        mongodb_cursor_info& create_cursor(index_info index) {
            auto code = index.code;
            auto itr = code_cursor_map_.find(code);
            auto id = get_next_cursor_id(itr);
            auto db_table = get_db_table(index);
            mongodb_cursor_info new_cursor(id, std::move(index), std::move(db_table));
            return add_cursor(itr, code, std::move(new_cursor));
        }

        void drop_db() {
            CYBERWAY_ASSERT(code_cursor_map_.empty(), driver_opened_cursors_exception, "ChainDB has opened cursors");

            code_cursor_map_.clear(); // close all opened cursors
            table_changed_value_map_.clear(); // cancel all pending changes

            auto db_list = mongo_conn_.list_databases();
            for (auto& db: db_list) {
                auto db_name = db["name"].get_utf8().value;
                if (!db_name.starts_with(get_system_code_name())) continue;

                mongo_conn_.database(db_name).drop();
            }
        }

        primary_key_t available_pk(const table_info& table) {
            apply_table_changes(table);

            auto cursor = get_db_table(table).find(
                make_document(kvp(get_scope_field_name(), get_scope_name(table))),
                options::find()
                    .sort(make_document(kvp(table.pk_order->field, -1)))
                    .limit(1));

            auto itr = cursor.begin();
            if (cursor.end() != itr) {
                return _detail::get_pk_value(table, *itr) + 1;
            }

            return 0;
        }

        variant value(const table_info& table, const primary_key_t pk) {
            auto info = find_changed_value(table, pk);
            if (nullptr != info) {
                switch(info->state) {
                    case changed_value_state::Insert:
                    case changed_value_state::Update:
                        return info->value;

                    case changed_value_state::Delete:
                    case changed_value_state::Unknown:
                        break;
                }
            } else {
                auto cursor = get_db_table(table).find(
                    _detail::make_pk_document(table, pk).view(),
                    options::find().limit(1));

                auto itr = cursor.begin();
                if (cursor.end() != itr) {
                    return build_variant(*itr);
                }
            }

            CYBERWAY_ASSERT(false, driver_absent_object_exception,
                "External database doesn't contain object with the primary key ${scope}.${pk} in the table ${table}",
                ("scope", get_scope_name(table))("pk", pk)("table", get_full_table_name(table)));
        }

        void insert(const table_info& table, const primary_key_t pk, variant value) {
            auto loc = get_changed_value(table, pk);
            auto& info = loc.info();

            switch (info.state) {
                case changed_value_state::Unknown:
                case changed_value_state::Delete: {
                    info.state = changed_value_state::Insert;
                    info.value = std::move(value);
                    break;
                }

                case changed_value_state::Insert:
                case changed_value_state::Update:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on insert");
            }
        }

        void update(const table_info& table, const primary_key_t pk, variant value) {
            auto loc = get_changed_value(table, pk);
            auto& info = loc.info();

            switch (info.state) {
                case changed_value_state::Unknown:
                    info.state = changed_value_state::Update;

                case changed_value_state::Insert:
                case changed_value_state::Update:
                    info.value = std::move(value);
                    break;

                case changed_value_state::Delete:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on update");
            }
        }

        void remove(const table_info& table, const primary_key_t pk) {
            auto loc = get_changed_value(table, pk);
            auto& info = loc.info();

            switch (info.state) {
                case changed_value_state::Update:
                    info.value = {};

                case changed_value_state::Unknown:
                    info.state = changed_value_state::Delete;
                    break;

                case changed_value_state::Insert:
                    loc.table().map.erase(loc.value_itr_);
                    break;

                case changed_value_state::Delete:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on delete");
            }
        }

    private:
        static mongocxx::instance& init_instance() {
            static mongocxx::instance instance;
            return instance;
        }

        collection get_db_table(const table_info& table) {
            return mongo_conn_[get_code_name(table)][get_table_name(table)];
        }

        cursor_t get_next_cursor_id(code_cursor_map::iterator itr) {
            if (itr != code_cursor_map_.end() && !itr->second.empty()) {
                return itr->second.rbegin()->second.id + 1;
            }
            return 1;
        }

        mongodb_cursor_info& add_cursor(
            code_cursor_map::iterator itr, const account_name& code, mongodb_cursor_info cursor
        ) {
            if (code_cursor_map_.end() == itr) {
                itr = code_cursor_map_.emplace(code, cursor_map()).first;
            }
            return itr->second.emplace(cursor.id, std::move(cursor)).first->second;
        }

        void apply_table_changes(const table_info& table) {
            auto begin_itr = table_changed_value_map_.lower_bound(std::make_tuple(table.code, table.table->name));
            auto end_itr = table_changed_value_map_.upper_bound(std::make_tuple(table.code, table.table->name));

            apply_range_changes(begin_itr, end_itr);
        }

        changed_value_info* find_changed_value(const table_info& table, const primary_key_t pk) {
            auto table_itr = table_object::find(table_changed_value_map_, table);
            if (table_changed_value_map_.end() == table_itr) return nullptr;

            auto& map = const_cast<table_changed_object&>(*table_itr).map;
            auto value_itr = map.find(pk);
            if (map.end() == value_itr) return nullptr;

            return &value_itr->second;
        }

        changed_value_location get_changed_value(const table_info& table, const primary_key_t pk) {
            auto table_itr = table_object::find(table_changed_value_map_, table);
            if (table_changed_value_map_.end() == table_itr) {
                table_itr = table_changed_value_map_.emplace(table).first;
            }

            auto& map = const_cast<table_changed_object&>(*table_itr).map;
            auto value_itr = map.find(pk);
            if (map.end() == value_itr) {
                value_itr = map.emplace(pk, changed_value_info()).first;
            }
            return {table_itr, value_itr};
        }

        cursor_location get_cursor(const cursor_request& request) {
            auto code_itr = code_cursor_map_.find(request.code);
            CYBERWAY_ASSERT(code_cursor_map_.end() != code_itr, driver_invalid_cursor_exception,
                "Cursor ${code}.${id} doesn't exist", ("code", get_code_name(request))("id", request.id));

            auto& map = code_itr->second;
            auto cursor_itr = map.find(request.id);
            CYBERWAY_ASSERT(map.end() != cursor_itr, driver_invalid_cursor_exception,
                "Cursor ${code}.${id} doesn't exist", ("code", get_code_name(request))("id", request.id));

            return cursor_location{code_itr, cursor_itr};
        }

        void apply_range_changes(table_changed_object_index::iterator itr, table_changed_object_index::iterator end) {
            std::string error;
            static auto bulk_options = options::bulk_write().ordered(false);
            std::unique_ptr<bulk_write> bulk_ptr;
            account_name code;
            table_name table;

            auto execute_bulk = [&]() {
                if (!bulk_ptr) return;

                _detail::auto_reconnect([&]() { try {
                    bulk_ptr->execute();
                } catch (const mongocxx::bulk_write_exception& e) {
                    error = e.what();
                    elog("Error on bulk write: ${what}", ("what", error));

                    if (_detail::get_mongo_code(e) != mongo_code::Duplicate) throw;
                }});
            };

            auto init_bulk = [&]() {
                if (code == itr->code && table == itr->table) return;

                execute_bulk();

                code = itr->code;
                table = itr->table;
                auto db_table = mongo_conn_.database(get_code_name(code)).collection(get_table_name(table));

                bulk_ptr = std::make_unique<bulk_write>(db_table.create_bulk_write(bulk_options));
            };

            auto append_bulk = [&](const primary_key_t pk, const changed_value_info& info) {
                auto& bulk = *bulk_ptr.get();
                auto& table = itr->info();
                switch (info.state) {
                    case changed_value_state::Delete:
                        bulk.append(delete_one(_detail::make_pk_document(table, pk).view()));
                        break;

                    case changed_value_state::Insert: {
                        document insert;
                        build_document(insert, info.value.get_object());

                        bulk.append(insert_one(insert.view()));
                        break;
                    }

                    case changed_value_state::Update: {
                        document update;
                        build_document(update, info.value.get_object());

                        bulk.append(update_one(
                            _detail::make_pk_document(table, pk).view(),
                            make_document(kvp("$set", update))));
                        break;
                    }

                    case changed_value_state::Unknown:
                        CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on apply change to ChainDB");
                }
            };

            for (; end != itr; table_changed_value_map_.erase(itr++)) {
                auto map = std::move(itr->map);
                if (map.empty()) continue;

                init_bulk();
                for (auto& key_value: map) {
                    append_bulk(key_value.first, key_value.second);
                }
            }

            execute_bulk();
            CYBERWAY_ASSERT(error.empty(), driver_duplicate_exception, error);
        }
    }; // struct mongodb_driver::mongodb_impl_

    mongodb_driver::mongodb_driver(const std::string& p)
    : impl_(new mongodb_impl_(p)) {
    }

    mongodb_driver::~mongodb_driver() = default;

    void mongodb_driver::drop_db() {
        impl_->drop_db();
    }

    const cursor_info& mongodb_driver::clone(const cursor_request& request) {
        return impl_->clone_cursor(request);
    }

    void mongodb_driver::close(const cursor_request& request) {
        impl_->close_cursor(request);
    }

    void mongodb_driver::close_all_cursors(const account_name& code) {
        impl_->close_all_cursors(code);
    }

    void mongodb_driver::apply_changes(const account_name& code) {
        impl_->apply_code_changes(code);
    }

    void mongodb_driver::apply_changes() {
        impl_->apply_all_changes();
    }

    void mongodb_driver::verify_table_structure(const table_info& table, const microseconds& max_time) {
        impl_->verify_table_structure(table, max_time);
    }

    const cursor_info& mongodb_driver::lower_bound(index_info index, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(_detail::start_from(), std::move(key));
        return cursor;
    }

    const cursor_info& mongodb_driver::upper_bound(index_info index, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(_detail::start_after(), std::move(key));
        return cursor;
    }

    const cursor_info& mongodb_driver::find(index_info index, primary_key_t pk, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(_detail::start_from(), std::move(key), pk);
        return cursor;
    }

    const cursor_info& mongodb_driver::end(index_info index) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open_end();
        return cursor;
    }

    const cursor_info& mongodb_driver::opt_find_by_pk(index_info index, primary_key_t pk, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open_by_pk(_detail::start_from(), std::move(key), pk);
        return cursor;
    }

    const cursor_info& mongodb_driver::current(const cursor_info& info) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        cursor.current();
        return cursor;
    }

    const cursor_info& mongodb_driver::current(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request).cursor();
        cursor.current();
        return cursor;
    }

    const cursor_info& mongodb_driver::next(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request).cursor();
        cursor.next();
        return cursor;
    }

    const cursor_info& mongodb_driver::prev(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request).cursor();
        cursor.prev();
        return cursor;
    }

    primary_key_t mongodb_driver::available_pk(const table_info& table) {
        return impl_->available_pk(table);
    }

    variant mongodb_driver::value(const table_info& table, const primary_key_t pk) {
        return impl_->value(table, pk);
    }

    const variant& mongodb_driver::value(const cursor_info& info) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        return cursor.get_object_value();
    }

    void mongodb_driver::set_blob(const cursor_info& info, bytes blob) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        cursor.blob = std::move(blob);
    }

    primary_key_t mongodb_driver::insert(const table_info& table, const primary_key_t pk, variant value) {
        impl_->insert(table, pk, std::move(value));
        return pk;
    }

    primary_key_t mongodb_driver::update(const table_info& table, const primary_key_t pk, variant value) {
        impl_->update(table, pk, std::move(value));
        return pk;
    }

    primary_key_t mongodb_driver::remove(const table_info& table, primary_key_t pk) {
        impl_->remove(table, pk);
        return pk;
    }

} } // namespace cyberway::chaindb
