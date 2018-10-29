#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>

#include <eosio/chain/symbol.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/query_exception.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::name;
    using eosio::chain::symbol;

    using fc::optional;
    using fc::blob;
    using fc::variants;
    using fc::variant_object;
    using fc::mutable_variant_object;

    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::document;
    using bsoncxx::builder::basic::sub_document;
    using bsoncxx::builder::basic::sub_array;
    using bsoncxx::builder::basic::kvp;

    using bsoncxx::document::element;
    using document_view = bsoncxx::document::view;
    using array_view = bsoncxx::array::view;

    using bsoncxx::types::b_null;
    using bsoncxx::types::b_oid;
    using bsoncxx::types::b_bool;
    using bsoncxx::types::b_double;
    using bsoncxx::types::b_int64;
    using bsoncxx::types::b_binary;
    using bsoncxx::types::b_array;
    using bsoncxx::types::b_document;

    using bsoncxx::type;
    using bsoncxx::binary_sub_type;
    using bsoncxx::oid;

    using mongocxx::bulk_write;
    using mongocxx::model::insert_one;
    using mongocxx::model::update_one;
    using mongocxx::model::delete_one;
    using mongocxx::collection;
    using mongocxx::query_exception;
    namespace options = mongocxx::options;

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

    namespace { namespace _detail {

        variant build_variant(const document_view&);

        blob build_blob(const b_binary& src) {
            blob dst;
            if (src.sub_type != binary_sub_type::k_binary) return dst;
            dst.data.assign(src.bytes, src.bytes + src.size);
            return dst;
        }

        variants build_variant(const array_view& src) {
            variants dst;
            for (auto& item: src) {
                switch (item.type()) {
                    case type::k_null:
                        dst.emplace_back(variant());
                        break;
                    case type::k_int32:
                        dst.emplace_back(item.get_int32().value);
                        break;
                    case type::k_int64:
                        dst.emplace_back(item.get_int64().value);
                        break;
                    case type::k_decimal128:
                        // TODO
                        break;
                    case type::k_double:
                        dst.emplace_back(item.get_double().value);
                        break;
                    case type::k_utf8:
                        dst.emplace_back(item.get_utf8().value.to_string());
                        break;
                    case type::k_date:
                        // TODO
                        break;
                    case type::k_timestamp:
                        // TODO
                        break;
                    case type::k_document:
                        dst.emplace_back(build_variant(item.get_document().value));
                        break;
                    case type::k_array:
                        dst.emplace_back(build_variant(item.get_array().value));
                        break;
                    case type::k_binary:
                        dst.emplace_back(build_blob(item.get_binary()));
                        break;
                    case type::k_bool:
                        dst.emplace_back(item.get_bool().value);
                        break;

                        // SKIP
                    case type::k_code:
                    case type::k_codewscope:
                    case type::k_symbol:
                    case type::k_dbpointer:
                    case type::k_regex:
                    case type::k_oid:
                    case type::k_maxkey:
                    case type::k_minkey:
                    case type::k_undefined:
                        break;
                }
            }
            return dst;
        }

        void build_variant(mutable_variant_object& dst, string key, const element& src) {
            switch (src.type()) {
                case type::k_null:
                    dst.set(std::move(key), variant());
                    break;
                case type::k_int32:
                    dst.set(std::move(key), src.get_int32().value);
                    break;
                case type::k_int64:
                    dst.set(std::move(key), src.get_int64().value);
                    break;
                case type::k_decimal128:
                    // TODO
                    break;
                case type::k_double:
                    dst.set(std::move(key), src.get_double().value);
                    break;
                case type::k_utf8:
                    dst.set(std::move(key), src.get_utf8().value.to_string());
                    break;
                case type::k_date:
                    // TODO
                    break;
                case type::k_timestamp:
                    // TODO
                    break;
                case type::k_document:
                    dst.set(std::move(key), build_variant(src.get_document().value));
                    break;
                case type::k_array:
                    dst.set(std::move(key), build_variant(src.get_array().value));
                    break;
                case type::k_binary:
                    dst.set(std::move(key), build_blob(src.get_binary()));
                    break;
                case type::k_bool:
                    dst.set(std::move(key), src.get_bool().value);
                    break;

                    // SKIP
                case type::k_code:
                case type::k_codewscope:
                case type::k_symbol:
                case type::k_dbpointer:
                case type::k_regex:
                case type::k_oid:
                case type::k_maxkey:
                case type::k_minkey:
                case type::k_undefined:
                    break;
            }
        }

        variant build_variant(const document_view& src) {
            mutable_variant_object dst;
            for (auto& item: src) {
                build_variant(dst, item.key().to_string(), item);
            }
            return variant(std::move(dst));
        }

        sub_document& build_document(sub_document&, const variant_object&);

        b_binary build_binary(const blob& src) {
            auto size = uint32_t(src.data.size());
            auto data = (uint8_t*) src.data.data();
            return b_binary{binary_sub_type::k_binary, size, data};
        }

        sub_array& build_document(sub_array& dst, const variants& src) {
            for (auto& item: src) {
                switch (item.get_type()) {
                    case variant::null_type:
                        dst.append(b_null());
                        break;
                    case variant::int64_type:
                        dst.append(b_int64{item.as_int64()});
                        break;
                    case variant::uint64_type:
                        dst.append(b_int64{int64_t(item.as_uint64())});
                        break;
                        // TODO
                        // case variant::int128_type:
                        //     break;
                        // case variant::uint128_type:
                        //     break;
                    case variant::double_type:
                        dst.append(b_double{item.as_double()});
                        break;
                    case variant::bool_type:
                        dst.append(b_bool{item.as_bool()});
                        break;
                    case variant::string_type:
                        dst.append(item.as_string());
                        break;
                        // TODO
                        // case variant::time_point_type:
                        //     break;
                        // case variant::time_point_sec_type:
                        //     break;
                    case variant::array_type:
                        dst.append([&](sub_array array){ build_document(array, item.get_array()); });
                        break;
                    case variant::object_type:
                        dst.append([&](sub_document sub_doc){ build_document(sub_doc, item.get_object()); });
                        break;
                    case variant::blob_type:
                        dst.append(build_binary(item.as_blob()));
                        break;
                }
            }
            return dst;
        }

        sub_document& build_document(sub_document& dst, const string& key, const variant& src) {
            switch (src.get_type()) {
                case variant::null_type:
                    dst.append(kvp(key, b_null()));
                    break;
                case variant::int64_type:
                    dst.append(kvp(key, b_int64{src.as_int64()}));
                    break;
                case variant::uint64_type:
                    dst.append(kvp(key, b_int64{int64_t(src.as_uint64())}));
                    break;
                    // TODO
                    // case variant::int128_type:
                    //     break;
                    // case variant::uint128_type:
                    //     break;
                case variant::double_type:
                    dst.append(kvp(key, b_double{src.as_double()}));
                    break;
                case variant::bool_type:
                    dst.append(kvp(key, b_bool{src.as_bool()}));
                    break;
                case variant::string_type:
                    dst.append(kvp(key, src.as_string()));
                    break;
                    // TODO
                    // case variant::time_point_type:
                    //     break;
                    // case variant::time_point_sec_type:
                    //     break;
                case variant::array_type:
                    dst.append(kvp(key, [&](sub_array array){ build_document(array, src.get_array()); }));
                    break;
                case variant::object_type:
                    dst.append(kvp(key, [&](sub_document sub_doc){ build_document(sub_doc, src.get_object()); }));
                    break;
                case variant::blob_type:
                    dst.append(kvp(key, build_binary(src.as_blob())));
                    break;
            }
            return dst;
        }

        sub_document& build_document(sub_document& dst, const variant_object& src) {
            for (auto& item: src) {
                build_document(dst, item.key(), item.value());
            }
            return dst;
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
                CYBERWAY_ASSERT(object.end() != itr, driver_absent_field_exception, "");

                --pos;
                if (0 == pos) {
                    return itr->value();
                } else {
                    object = itr->value().get_object();
                }
            }
            CYBERWAY_ASSERT(false, driver_absent_field_exception, "");
        } catch (...) {
            CYBERWAY_ASSERT(false, driver_absent_field_exception,
                "External database returns the row in the table ${table} without the field ${field}.",
                ("table", get_full_table_name(index))("field", order.field));
        } }

        primary_key_t get_pk_value(const table_info& table, document_view view) { try {
            auto& pk_order = *table.pk_order;
            auto pos = pk_order.path.size();
            for (auto& key: pk_order.path) {
                --pos;
                auto itr = view.find(key);
                CYBERWAY_ASSERT(view.end() != itr, driver_primary_key_exception, "");

                if (pos == 0) {
                    switch (pk_order.type.front()) {
                        case 'i': // int64
                        case 'u': // uint64
                            return itr->get_int64().value;
                        case 'n': // name
                            return name(itr->get_utf8().value.data()).value;
                        case 's': // symbol_code
                            return symbol(0, itr->get_utf8().value.data()).value();
                        default:
                            break;
                    }
                } else {
                    view = itr->get_document().value;
                }
            }
            CYBERWAY_ASSERT(false, driver_primary_key_exception, "");
        } catch (...) {
            CYBERWAY_ASSERT(false, driver_primary_key_exception,
                "External database locate row in the table ${table} with wrong type of primary key.",
                ("table_name", get_full_table_name(table)));
        } }

        document make_pk_document(const table_info& table, primary_key_t pk) {
            document doc;
            doc.append(kvp(get_scope_field_name(), get_scope_name(table)));

            auto& pk_order = *table.pk_order;
            switch (pk_order.type.front()) {
                case 'i': // int64
                case 'u': // uint64
                    doc.append(kvp(pk_order.field, static_cast<int64_t>(pk)));
                    break;
                case 'n': // name
                    doc.append(kvp(pk_order.field, name(pk).to_string()));
                    break;
                case 's': // symbol_code
                    doc.append(kvp(pk_order.field, symbol(pk).name()));
                    break;
                default:
                    CYBERWAY_ASSERT(false, driver_primary_key_exception, "Invalid type of primary key");
            }

            return doc;
        }

    } } // namespace _detail

    class mongodb_cursor: public cursor_info {
    public:
        mongodb_cursor(cursor_t id, index_info index, collection db_table)
        : cursor_info{id, std::move(index)},
          db_table_(std::move(db_table))
        { }

        mongodb_cursor() = default;
        mongodb_cursor(mongodb_cursor&&) = default;

        mongodb_cursor(const mongodb_cursor&) = delete;

        mongodb_cursor clone(cursor_t id) {
            mongodb_cursor dst(id, index, db_table_);
            dst.pk = pk;
            dst.blob = blob;

            if (find_cmp_->order == direction::Forward) {
                dst.find_cmp_ = &_detail::start_from();
            } else {
                dst.find_cmp_ = &_detail::reverse_start_from();
            }

            if (source_.valid()) {
                // it is faster to get object from exist cursor then to open a new cursor, locate, and get object
                dst.find_key_ = get_key_value();
                dst.find_pk_ = get_pk_value();
                dst.object_ = get_object_value();
                dst.key_ = dst.find_key_;
            } else {
                dst.find_key_ = find_key_;
                dst.find_pk_ = find_pk_;
                dst.object_ = object_;
                dst.key_ = key_;
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

            find_cmp_ = &_detail::reverse_start_from();
            find_key_.clear();
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
            object_ = _detail::build_variant(view);
            pk = _detail::get_pk_value(index, view);

            return object_;
        }

        const variant& get_key_value() {
            lazy_open();
            if (is_end() || !key_.is_null()) return key_;

            auto& view = *source_->begin();
            key_ = _detail::build_variant(view);

            return key_;
        }

    private:
        collection db_table_;

        const cmp_info* find_cmp_ = &_detail::start_from();
        variant find_key_;
        primary_key_t find_pk_ = unset_primary_key;

        optional<mongocxx::cursor> source_;
        variant key_;
        variant object_;

        bool is_end() const {
            return end_primary_key == pk;
        }

        void change_direction(const cmp_info& find_cmp) {
            find_cmp_ = &find_cmp;
            if (source_.valid()) {
                find_key_ = get_key_value();
                find_pk_ = get_pk_value();
            }

            source_.reset();
        }

        void reset() {
            if (!object_.is_null()) object_.clear();
            if (!key_.is_null()) key_.clear();
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
                find.append(kvp(o.field, [&](sub_document doc){_detail::build_document(doc, cmp, value);}));
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

            source_.emplace(db_table_.find(find.view(), options::find().sort(sort.view())));

            const auto find_pk = find_pk_;
            find_pk_ = unset_primary_key;

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

    }; // class mongodb_cursor

    using cursor_map = std::map<cursor_t, mongodb_cursor>;

    struct code_info {
        cursor_map cursor_map;

        code_info() = default;
        code_info(code_info&&) = default;

        code_info& operator=(code_info&&) = default;

        ~code_info() {
            apply_changes();
        }

        bool empty() const {
            return cursor_map.empty() && tables_bulk_write_.empty();
        }

        void apply_changes() {
            for (auto& table: tables_bulk_write_) {
                table.second.execute();
            }
            tables_bulk_write_.clear();
        }

        void apply_changes(const table_name& table) {
            auto itr = tables_bulk_write_.find(table);
            if (tables_bulk_write_.end() != itr) {
                itr->second.execute();
                tables_bulk_write_.erase(itr);
            }
        }

        bulk_write& get_changes_buffer(const table_name& table, collection db_table) {
            auto itr = tables_bulk_write_.find(table);
            if (tables_bulk_write_.end() == itr) {
                itr = tables_bulk_write_.emplace(table, db_table.create_bulk_write(options::bulk_write().ordered(false))).first;
            }
            return itr->second;
        }

    private:
        std::map<table_name, bulk_write> tables_bulk_write_;
    }; // struct code_info

    using code_map = std::map<account_name, code_info>;

    struct cursor_location {
        mongodb_cursor& cursor;
        code_map::iterator code_itr;
        cursor_map::iterator cursor_itr;
        code_map& code_map;
        code_info& code_info;
    }; // struct cursor_location

    struct mongodb_driver::mongodb_impl_ {
        mongocxx::instance mongo_inst_;
        mongocxx::client mongo_conn_;
        code_map code_map_;

        mongodb_impl_(const std::string& p) {
            mongocxx::uri uri{p};
            mongo_conn_ = mongocxx::client{uri};
        }

        ~mongodb_impl_() = default;

        cursor_location get_cursor(const cursor_request& request) {
            auto code_itr = code_map_.find(request.code);
            CYBERWAY_ASSERT(
                code_map_.end() != code_itr, driver_invalid_cursor_exception,
                "Cursor ${code}.${id} doesn't exist", ("code", get_code_name(request))("id", request.id));

            auto& cursor_map = code_itr->second.cursor_map;
            auto cursor_itr = cursor_map.find(request.id);
            CYBERWAY_ASSERT(
                cursor_map.end() != cursor_itr, driver_invalid_cursor_exception,
                "Cursor ${code}.${id} doesn't exist", ("code", get_code_name(request))("id", request.id));

            return cursor_location{cursor_itr->second, code_itr, cursor_itr, code_map_, code_itr->second};
        }

        cursor_location get_applied_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            if (loc.cursor.pk == unset_primary_key) {
                apply_changes(loc.cursor.index);
            }
            return loc;
        }

        collection get_db_table(const table_info& table) {
            return mongo_conn_[get_code_name(table)][get_table_name(table)];
        }

        bulk_write& get_mutable_db_table(const table_info& table) {
            auto itr = code_map_.find(table.code);
            if (code_map_.end() == itr) {
                itr = code_map_.emplace(table.code, code_info()).first;
            }
            return itr->second.get_changes_buffer(table.table->name, get_db_table(table));
        }

        void apply_changes(const table_info& table) {
            auto itr = code_map_.find(table.code);
            if (code_map_.end() != itr) {
                itr->second.apply_changes(table.table->name);
                if (itr->second.empty()) {
                    code_map_.erase(itr);
                }
            }
        }

        void apply_changes() {
            for (auto itr = code_map_.begin(), etr = code_map_.end(); etr != itr;) {
                itr->second.apply_changes();
                if (itr->second.empty()) {
                    code_map_.erase(itr++);
                } else {
                    ++itr;
                }
            }
        }

        cursor_t get_next_cursor_id(code_map::iterator itr) {
            if (itr != code_map_.end() && !itr->second.cursor_map.empty()) {
                return itr->second.cursor_map.rbegin()->second.id + 1;
            }
            return 1;
        }

        mongodb_cursor& add_cursor(code_map::iterator itr, const account_name& code, mongodb_cursor cursor) {
            if (code_map_.end() == itr) {
                itr = code_map_.emplace(code, code_info()).first;
            }
            return itr->second.cursor_map.emplace(cursor.id, std::move(cursor)).first->second;
        }

        void close_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            loc.code_info.cursor_map.erase(loc.cursor_itr);
            if (loc.code_info.empty()) {
                loc.code_map.erase(loc.code_itr);
            }
        }

        mongodb_cursor& clone_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            auto next_id = get_next_cursor_id(loc.code_itr);

            auto cloned_cursor = loc.cursor.clone(next_id);
            return add_cursor(loc.code_itr, request.code, std::move(cloned_cursor));
        }

        void verify_table_structure(const table_info& table, const microseconds& max_time) {
            auto db_table = get_db_table(table);
            auto& indexes = table.table->indexes;

            for (auto& index: indexes) {
                bool was_pk = false;
                document doc;
                doc.append(kvp(get_scope_field_name(), 1));
                for (auto& o: index.orders) {
                    if (_detail::is_asc_order(o.order)) {
                        doc.append(kvp(o.field, 1));
                    } else {
                        doc.append(kvp(o.field, -1));
                    }
                    was_pk |= (&o == table.pk_order);
                }
                if (!was_pk && !index.unique) {
                    doc.append(kvp(table.pk_order->field, 1));
                }

                auto db_index_name = get_index_name(index);
                try {
                    db_table.create_index(doc.view(), options::index().name(db_index_name).unique(index.unique));
                } catch (const mongocxx::operation_exception& e) {
                    db_table.indexes().drop_one(db_index_name);
                    db_table.create_index(doc.view(), options::index().name(db_index_name).unique(index.unique));
                }
            }
        }

        mongodb_cursor& create_cursor(index_info index) {
            auto code = index.code;
            auto itr = code_map_.find(code);
            auto id = get_next_cursor_id(itr);
            auto db_table = get_db_table(index);
            mongodb_cursor new_cursor(id, std::move(index), std::move(db_table));
            return add_cursor(itr, code, std::move(new_cursor));
        }
    };

    mongodb_driver::mongodb_driver(const std::string& p)
    : impl_(new mongodb_impl_(p))
    { }

    mongodb_driver::~mongodb_driver() = default;

    const cursor_info& mongodb_driver::clone(const cursor_request& request) {
        return impl_->clone_cursor(request);
    }

    void mongodb_driver::close(const cursor_request& request) {
        impl_->close_cursor(request);
    }

    void mongodb_driver::close_all_cursors(const account_name& code) {
        impl_->code_map_.erase(code);
    }

    void mongodb_driver::apply_changes() {
        impl_->apply_changes();
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
        auto& cursor = const_cast<mongodb_cursor&>(static_cast<const mongodb_cursor&>(info));
        cursor.current();
        return cursor;
    }

    const cursor_info& mongodb_driver::current(const cursor_request& request) {
        auto loc = impl_->get_applied_cursor(request);
        loc.cursor.current();
        return loc.cursor;
    }

    const cursor_info& mongodb_driver::next(const cursor_request& request) {
        auto loc = impl_->get_applied_cursor(request);
        loc.cursor.next();
        return loc.cursor;
    }

    const cursor_info& mongodb_driver::prev(const cursor_request& request) {
        auto loc = impl_->get_applied_cursor(request);
        loc.cursor.prev();
        return loc.cursor;
    }

    variant mongodb_driver::value(const table_info& table, const primary_key_t pk) {
        impl_->apply_changes(table);

        auto cursor = impl_->get_db_table(table).find(
            _detail::make_pk_document(table, pk).view(),
            options::find().limit(1));

        auto itr = cursor.begin();
        CYBERWAY_ASSERT(cursor.end() != itr, driver_absent_object_exception,
            "External database doesn't contain object with the primary key ${pk} in the table ${table}",
            ("pk", pk)("table", get_full_table_name(table)));

        return _detail::build_variant(*itr);
    }

    const variant& mongodb_driver::value(const cursor_info& info) {
        auto& cursor = const_cast<mongodb_cursor&>(static_cast<const mongodb_cursor&>(info));
        return cursor.get_object_value();
    }

    void mongodb_driver::set_blob(const cursor_info& info, bytes blob) {
        auto& cursor = const_cast<mongodb_cursor&>(static_cast<const mongodb_cursor&>(info));
        cursor.blob = std::move(blob);
    }

    primary_key_t mongodb_driver::available_pk(const table_info& table) {
        impl_->apply_changes(table);

        auto cursor = impl_->get_db_table(table).find(
            make_document(kvp(get_scope_field_name(), get_scope_name(table))),
            options::find()
                .sort(make_document(kvp(table.pk_order->field, -1)))
                .limit(1));

        auto itr = cursor.begin();
        if (cursor.end() != itr) {
            return _detail::get_pk_value(table, *itr) + 1;
        }

        return 1;
    }

    primary_key_t mongodb_driver::insert(const table_info& table, const primary_key_t pk, const variant& value) {
        auto& object = value.get_object();

        document insert;
        _detail::build_document(insert, object);

        impl_->get_mutable_db_table(table).append(insert_one(insert.view()));

//        std::cout << bsoncxx::to_json(insert.view()) << std::endl;

//        auto inserted = impl_->get_db_table(table).insert_one(insert.view());
//        CYBERWAY_ASSERT(inserted && inserted->result().inserted_count() == 1, driver_insert_exception,
//            "Fail to insert object ${object} into the table ${table}",
//            ("object", object)("table", get_full_table_name(table)));

        return pk;
    }

    primary_key_t mongodb_driver::update(const table_info& table, const primary_key_t pk, const variant& value) {
        auto& object = value.get_object();

        document update;
        _detail::build_document(update, object);

        impl_->get_mutable_db_table(table).append(update_one(
            _detail::make_pk_document(table, pk).view(),
            make_document(kvp("$set", update))));

//        auto updated = impl_->get_db_table(table).update_one(
//            _detail::make_pk_document(table, pk),
//            make_document(kvp("$set", update)));
//
//        CYBERWAY_ASSERT(updated && updated->result().matched_count() == 1, driver_update_exception,
//            "Fail to update object ${object} in the table ${table}",
//            ("object", object)("table", get_full_table_name(table)));

        return pk;
    }

    primary_key_t mongodb_driver::remove(const table_info& table, primary_key_t pk) {
        impl_->get_mutable_db_table(table).append(delete_one(
            _detail::make_pk_document(table, pk).view()));

//        auto deleted = impl_->get_db_table(table).delete_one(doc.view());
//
//        CYBERWAY_ASSERT(deleted && deleted->result().deleted_count() == 1, driver_update_exception,
//            "Fail to delete object ${object} from the table ${table}",
//            ("object", bsoncxx::to_json(doc.view()))("table", get_full_table_name(table)));

        return pk;
    }

} } // namespace cyberway::chaindb
