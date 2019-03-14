#include <limits>

#include <cyberway/chaindb/mongo_big_int_converter.hpp>
#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/table_object.hpp>
#include <cyberway/chaindb/mongo_driver_utils.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/query_exception.hpp>
#include <cyberway/chaindb/abi_info.hpp>

namespace cyberway { namespace chaindb {

    using fc::variant_object;

    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::document;
    using bsoncxx::builder::basic::sub_document;
    using bsoncxx::builder::basic::kvp;

    using mongocxx::bulk_write;
    using mongocxx::model::insert_one;
    using mongocxx::model::replace_one;
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
        EmptyBulk = 22,
        DuplicateValue = 11000,
        NoServer = 13053
    };

    namespace { namespace _detail {
        static const string pk_index_postfix = "_pk";
        static const string mongodb_id_index = "_id_";
        static const string mongodb_indexes  = "system.indexes";

        template <typename Exception>
        mongo_code get_mongo_code(const Exception& e) {
            auto value = static_cast<mongo_code>(e.code().value());

            switch (value) {
                case mongo_code::EmptyBulk:
                case mongo_code::DuplicateValue:
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
            return name.size() == names::asc_order.size();
        }

        variant get_order_value(const variant_object& row, const index_info& index, const order_def& order) { try {
            auto* object = &row;
            auto pos = order.path.size();
            for (auto& key: order.path) {
                auto itr = object->find(key);
                CYBERWAY_ASSERT(object->end() != itr, driver_absent_field_exception,
                    "Can't find the part ${key} for the field ${field} in the row ${row} "
                    "from the table ${table} for the scope '${scope} ",
                    ("key", key)("field", order.field)
                    ("table", get_full_table_name(index))("scope", get_scope_name(index))("row", row));

                --pos;
                if (0 == pos) {
                    return itr->value();
                } else {
                    object = &itr->value().get_object();
                }
            }
            CYBERWAY_ASSERT(false, driver_absent_field_exception,
                "Wrong logic on parsing of the field ${field} in the row ${row} "
                "from the table ${table} for the scope '${scope}'",
                ("table", get_full_table_name(index))("field", order.field)("row", row));
        } catch (const driver_absent_field_exception&) {
            throw;
        } catch (...) {
            CYBERWAY_ASSERT(false, driver_absent_field_exception,
                "External database can't read the field ${field} in the row ${row} "
                "from the table ${table} for the scope '${scope}'",
                ("field", order.field)
                ("table", get_full_table_name(index))("scope", get_scope_name(index))("row", row));
        } }

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

            if (find_cmp_->order == direction::Forward) {
                dst.find_cmp_ = &_detail::start_from();
            } else {
                dst.find_cmp_ = &_detail::reverse_start_from();
            }

            if (source_) {
                // it is faster to get object from exist cursor then to open a new cursor, locate, and get object
                dst.object_ = get_object_value();
                dst.find_key_ = dst.object_.value;
                dst.find_pk_ = get_pk_value();
            } else {
                dst.find_key_ = find_key_;
                dst.find_pk_ = find_pk_;
                dst.object_ = object_;
            }
            dst.pk = pk;
            dst.blob = blob;

            return dst;
        }

        mongodb_cursor_info& open(const cmp_info& find_cmp, variant key, const primary_key_t locate_pk = unset_primary_key) {
            reset();

            find_cmp_ = &find_cmp;
            find_key_ = std::move(key);
            find_pk_ = locate_pk;

            return *this;
        }

        mongodb_cursor_info& open_by_pk(variant key, const primary_key_t locate_pk) {
            reset();
            pk = locate_pk;

            find_cmp_ = &_detail::start_from();
            find_key_ = std::move(key);
            find_pk_ = unset_primary_key;

            return *this;
        }

        mongodb_cursor_info& open_begin() {
            reset();

            find_cmp_ = &_detail::start_from();
            find_key_.clear();
            find_pk_ = unset_primary_key;

            return *this;
        }

        mongodb_cursor_info& open_end() {
            reset();
            pk = end_primary_key;

            find_cmp_ = &_detail::reverse_start_from();
            find_key_.clear();
            find_pk_ = unset_primary_key;

            return *this;
        }

        mongodb_cursor_info& open_end_() {
            reset();
            pk = end_primary_key;

            find_cmp_ = &_detail::reverse_start_from();
            find_key_.clear();
            find_pk_ = unset_primary_key;

            return *this;
        }

        void next() {
            if (find_cmp_->order == direction::Backward) {
                change_direction(_detail::start_from());
            }
            lazy_next();
        }

        void prev() {
            if (end_primary_key == pk) {
                open_end_();
                pk = unset_primary_key;
                lazy_open();
                return;
            } else if (find_cmp_->order == direction::Forward) {
                change_direction(_detail::reverse_start_from());
            }
            lazy_next();
        }

        void current() {
            lazy_open();
        }

        const object_value& get_object_value() {
            lazy_open();
            if (!object_.value.is_null()) return object_;

            if (end_primary_key == get_pk_value()) {
                object_.value = variant_object();
                object_.service.pk    = pk;
                object_.service.code  = index.code;
                object_.service.scope = index.scope;
                object_.service.table = index.table->name;
            } else {
                auto& view = *source_->begin();
                object_ = build_object(index, view);
                pk = object_.service.pk;
            }

            return object_;
        }

    private:
        collection db_table_;

        const cmp_info* find_cmp_ = &_detail::start_from();
        variant find_key_;
        primary_key_t find_pk_ = unset_primary_key;

        std::unique_ptr<mongocxx::cursor> source_;
        object_value object_;

        void change_direction(const cmp_info& find_cmp) {
            find_cmp_ = &find_cmp;
            if (source_) {
                find_key_ = get_object_value().value;
                find_pk_ = get_pk_value();
            }

            reset();
        }

        void reset_object() {
            pk = unset_primary_key;
            if (!blob.empty()) blob.clear();
            if (!object_.is_null()) object_.clear();
        }

        void reset() {
            reset_object();
            source_.reset();
        }

        document create_find_document(const char* forward, const char* backward) const {
            document find;

            find.append(kvp(names::scope_path, get_scope_name(index)));
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
            if (end_primary_key == pk) return;

            reset();

            auto find = create_find_document(find_cmp_->from_forward, find_cmp_->from_backward);
            auto sort = create_sort_document();

            const auto find_pk = find_pk_;
            find_pk_ = unset_primary_key;

            _detail::auto_reconnect([&]() {
                source_ = std::make_unique<mongocxx::cursor>(db_table_.find(find.view(), options::find().sort(sort.view())));

                init_pk_value();
                if (unset_primary_key == find_pk || find_pk == pk || end_primary_key == pk) return;
                if (index.index->unique || !find_cmp_->to_forward) return;

                // locate cursor to primary key

                auto to_find = create_find_document(find_cmp_->to_forward, find_cmp_->to_backward);
                auto to_cursor = db_table_.find(to_find.view(), options::find().sort(sort.view()).limit(1));

                auto to_pk = unset_primary_key;
                auto to_itr = to_cursor.begin();
                if (to_itr != to_cursor.end()) to_pk = chaindb::get_pk_value(index, *to_itr);

                // TODO: limitation by deadline
                static constexpr int max_iters = 10000;
                auto itr = source_->begin();
                auto etr = source_->end();
                for (int i = 0; i < max_iters && itr != etr; ++i, ++itr) {
                    pk = chaindb::get_pk_value(index, *to_itr);
                    // range is end, but pk not found
                    if (to_pk == pk) break;
                    // ok, key is found
                    if (find_pk == pk) return;
                }

                open_end();
            });
        }

        void lazy_next() {
            reset_object();
            lazy_open();
            if (!source_) return;

            auto itr = source_->begin();
            if (source_->end() != itr) {
                ++itr;
            }
            CYBERWAY_ASSERT(find_cmp_->order != direction::Backward || itr != source_->end(),
                driver_out_of_range_exception,
                "External database tries to locate row in the index ${index} for the scope '${scope}' in out of range",
                ("index", get_full_index_name(index))("scope", get_scope_name(index)));
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
                pk = end_primary_key;
            } else {
                pk = chaindb::get_pk_value(index, *itr);
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

    ///----

    struct mongodb_driver::mongodb_impl_ {
        journal& journal_;
        string sys_code_name_;
        mongocxx::client mongo_conn_;
        code_cursor_map code_cursor_map_;

        mongodb_impl_(journal& jrnl, string address, string sys_name)
        : journal_(jrnl),
          sys_code_name_(std::move(sys_name)) {
            init_instance();
            mongocxx::uri uri{address};
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
            journal_.apply_code_changes(write_ctx_t_(*this), code);
        }

        void apply_all_changes() {
            journal_.apply_all_changes(write_ctx_t_(*this));
        }

        void close_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            auto& map = loc.map();

            map.erase(loc.cursor_itr_);
            if (map.empty()) {
                code_cursor_map_.erase(loc.code_itr_);
            }
        }

        void close_code_cursors(const account_name& code) {
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

        std::vector<index_def> get_db_indexes(collection& db_table) const {
            std::vector<index_def> result;
            auto indexes = db_table.list_indexes();

            result.reserve(abi_info::max_index_cnt * 2);
            for (auto& info: indexes) {
                index_def index;

                auto itr = info.find("name");
                if (info.end() == itr) continue;
                auto iname = itr->get_utf8().value;
                try {
                    index.name = index_name(iname.data());
                } catch (const eosio::chain::name_type_exception&) {
                    if (iname.ends_with(_detail::pk_index_postfix)) continue;
                    if (iname.compare(  _detail::mongodb_id_index)) db_table.indexes().drop_one(iname);
                    continue;
                }

                itr = info.find("unique");
                if (info.end() != itr) index.unique = itr->get_bool().value;

                itr = info.find("key");
                if (info.end() != itr) {
                    auto fields = itr->get_document().value;
                    for (auto& field: fields) {
                        order_def order;

                        auto key = field.key();
                        if (!key.compare(names::scope_path)) continue;

                        order.field = key.to_string();

                        // <FIELD_NAME>.binary - we should remove ".binary"
                        if (order.field.size() > mongo_big_int_converter::BINARY_FIELD.size() &&
                            key.ends_with(mongo_big_int_converter::BINARY_FIELD)
                        ) {
                            order.field.erase(order.field.size() - 1 - mongo_big_int_converter::BINARY_FIELD.size());
                        }

                        order.order = field.get_int32().value == 1 ? names::asc_order : names::desc_order;
                        index.orders.emplace_back(std::move(order));
                    }

                    // see create_index()
                    if (!index.unique) index.orders.pop_back();
                }
                result.emplace_back(std::move(index));
            }
            return result;
        }

        std::vector<table_def> db_tables(const account_name& code) const {
            static constexpr std::chrono::milliseconds max_time(10);
            std::vector<table_def> tables;

            tables.reserve(abi_info::max_table_cnt * 2);
            _detail::auto_reconnect([&]() {
                tables.clear();
                auto db = mongo_conn_.database(get_code_name(sys_code_name_, code));
                auto names = db.list_collection_names();
                for (auto& tname: names) {
                    table_def table;

                    try {
                        table.name = table_name(tname);
                    } catch (const eosio::chain::name_type_exception&) {
                        if (tname != _detail::mongodb_indexes) db.collection(tname).drop();
                        continue;
                    }
                    auto db_table = db.collection(tname);
                    table.row_count = db_table.estimated_document_count(
                        options::estimated_document_count().max_time(max_time));
                    table.indexes = get_db_indexes(db_table);

                    tables.emplace_back(std::move(table));
                }
            });

            return tables;
        }

        void drop_index(const index_info& info) const {
            get_db_table(info).indexes().drop_one(get_index_name(info));
        }

        void drop_table(const table_info& info) const {
            get_db_table(info).drop();
        }

        void create_index(const index_info& info) const {
            document idx_doc;
            bool was_pk = false;
            auto& index = *info.index;

            idx_doc.append(kvp(names::scope_path, 1));
            for (auto& order: index.orders) {
                auto field = order.field;
                // ui128 || i128
                if (order.type.back() == '8' && (order.type.front() == 'i' || order.type.front() == 'u')) {
                    field.append(".").append(mongo_big_int_converter::BINARY_FIELD);
                }
                if (_detail::is_asc_order(order.order)) {
                    idx_doc.append(kvp(field, 1));
                } else {
                    idx_doc.append(kvp(field, -1));
                }
                was_pk |= (&order == info.pk_order);
            }

            if (!was_pk && !index.unique) {
                // when index is not unique, we add unique pk for deterministic order of records
                idx_doc.append(kvp(info.pk_order->field, 1));
            }

            auto idx_name = get_index_name(info);
            auto db_table = get_db_table(info);
            db_table.create_index(idx_doc.view(), options::index().name(idx_name).unique(index.unique));

            // for available primary key
            if (info.pk_order == &index.orders.front()) {
                document id_doc;
                idx_name.append(_detail::pk_index_postfix);
                id_doc.append(kvp(info.pk_order->field, 1));
                db_table.create_index(id_doc.view(), options::index().name(idx_name));
            }
        }

        mongodb_cursor_info& create_cursor(index_info index) {
            auto code = index.code;
            auto itr = code_cursor_map_.find(code);
            auto id = get_next_cursor_id(itr);
            auto db_table = get_db_table(index);
            mongodb_cursor_info new_cursor(id, std::move(index), std::move(db_table));
            apply_table_changes(index);
            return add_cursor(std::move(itr), code, std::move(new_cursor));
        }

        void drop_db() {
            CYBERWAY_ASSERT(code_cursor_map_.empty(), driver_opened_cursors_exception, "ChainDB has opened cursors");

            code_cursor_map_.clear(); // close all opened cursors

            auto db_list = mongo_conn_.list_databases();
            for (auto& db: db_list) {
                auto db_name = db["name"].get_utf8().value;
                if (!db_name.starts_with(sys_code_name_)) continue;

                mongo_conn_.database(db_name).drop();
            }
        }

        primary_key_t available_pk(const table_info& table) {
            apply_table_changes(table);
            primary_key_t pk = 0;

            _detail::auto_reconnect([&] {
                auto cursor = get_db_table(table).find(
                    make_document(),
                    options::find()
                        .sort(make_document(kvp(table.pk_order->field, -1)))
                        .limit(1));

                auto itr = cursor.begin();
                if (cursor.end() != itr) {
                    pk = chaindb::get_pk_value(table, *itr) + 1;
                }
            });

            return pk;
        }

        object_value object_by_pk(const table_info& table, const primary_key_t pk) {
            document pk_doc;
            object_value obj;

            apply_table_changes(table);

            obj.service.pk = pk;
            build_find_pk_document(pk_doc, table, obj);
            auto cursor = get_db_table(table).find(pk_doc.view(), options::find().limit(1));

            auto itr = cursor.begin();
            if (cursor.end() != itr) {
                return build_object(table, *itr);
            } else {
                obj.value = variant_object();
                return obj;
            }

            CYBERWAY_ASSERT(false, driver_absent_object_exception,
                "External database doesn't contain object with the primary key ${pk} "
                "in the table ${table} for the scope '${scope}'",
                ("pk", pk)("table", get_full_table_name(table))("scope", get_scope_name(table)));
        }

    private:
        static mongocxx::instance& init_instance() {
            static mongocxx::instance instance;
            return instance;
        }

        collection get_db_table(const table_info& table) const {
            return mongo_conn_.database(get_code_name(sys_code_name_, table.code)).collection(get_table_name(table));
        }

        collection get_undo_db_table() const {
            return mongo_conn_.database(get_code_name(sys_code_name_, account_name())).collection(names::undo_table);
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
            journal_.apply_table_changes(write_ctx_t_(*this), table);
        }

        cursor_location get_cursor(const cursor_request& request) {
            auto code_itr = code_cursor_map_.find(request.code);
            CYBERWAY_ASSERT(code_cursor_map_.end() != code_itr, driver_invalid_cursor_exception,
                "The map for the cursor ${code}.${id} doesn't exist", ("code", get_code_name(request))("id", request.id));

            auto& map = code_itr->second;
            auto cursor_itr = map.find(request.id);
            CYBERWAY_ASSERT(map.end() != cursor_itr, driver_invalid_cursor_exception,
                "The cursor ${code}.${id} doesn't exist", ("code", get_code_name(request))("id", request.id));

            return cursor_location{code_itr, cursor_itr};
        }

        class write_ctx_t_ final {
            struct bulk_info_t_ final {
                bulk_write bulk_;
                int op_cnt_ = 0;

                bulk_info_t_(bulk_write bulk): bulk_(std::move(bulk)) { }
            }; // struct bulk_info_t_;

        public:
            write_ctx_t_(mongodb_impl_& impl)
            : impl_(impl) {
            }

            bulk_info_t_ create_bulk_info(collection db_table) {
                static options::bulk_write opts(options::bulk_write().ordered(false));
                return bulk_info_t_(db_table.create_bulk_write(opts));
            }

            void init() {
                data_bulk_list_.emplace_back(create_bulk_info(impl_.get_undo_db_table()));
                complete_undo_bulk_ = std::make_unique<bulk_info_t_>(create_bulk_info(impl_.get_undo_db_table()));
            }

            void start_table(const table_info& table) {
                auto old_table = table_;
                table_ = &table;
                if (BOOST_LIKELY(
                        old_table != nullptr &&
                        table.code == old_table->code &&
                        table.table->name == old_table->table->name))
                    return;

                data_bulk_list_.emplace_back(create_bulk_info(impl_.get_db_table(table)));
            }

            void add_data(const write_operation& op) {
                append_bulk(build_find_pk_document, build_service_document, data_bulk_list_.back(), op);
            }

            void add_prepare_undo(const write_operation& op) {
                append_bulk(build_find_undo_pk_document, build_undo_service_document, data_bulk_list_.front(), op);
            }

            void add_complete_undo(const write_operation& op) {
                append_bulk(build_find_undo_pk_document, build_undo_service_document, *complete_undo_bulk_, op);
            }

            void write() {
                for (auto& data_bulk: data_bulk_list_) {
                    execute_bulk(data_bulk);
                }
                execute_bulk(*complete_undo_bulk_);

                CYBERWAY_ASSERT(error_.empty(), driver_duplicate_exception, error_);
            }

        private:
            mongodb_impl_& impl_;
            std::string error_;
            const table_info* table_ = nullptr;
            std::unique_ptr<bulk_info_t_> complete_undo_bulk_;
            std::deque<bulk_info_t_> data_bulk_list_;

            template <typename BuildFindDocument, typename BuildServiceDocument>
            void append_bulk(
                BuildFindDocument&& build_find_document, BuildServiceDocument&& build_service_document,
                bulk_info_t_& info, const write_operation& op
            ) {
                document data_doc;
                document pk_doc;

                switch (op.operation) {
                    case write_operation::Insert:
                    case write_operation::Update:
                        build_document(data_doc, op.object);

                    case write_operation::Revision:
                        build_service_document(data_doc, *table_, op.object);

                    case write_operation::Remove:
                        build_find_document(pk_doc, *table_, op.object);
                        if (op.find_revision >= start_revision) {
                            pk_doc.append(kvp(names::revision_path, op.find_revision));
                        }
                        break;

                    case write_operation::Unknown:
                        CYBERWAY_ASSERT(false, driver_write_exception,
                            "Wrong operation type on writing into the table ${table} for the scope '${scope}"
                            "with the revision (find: ${find_rev}, set: ${set_rev}) and with the primary key ${pk}",
                            ("table", get_full_table_name(*table_))("scope", get_scope_name(*table_))
                            ("find_rev", op.find_revision)("set_rev", op.object.service.revision)("pk", op.object.pk()));
                        return;
                }

                ++info.op_cnt_;

                switch(op.operation) {
                    case write_operation::Insert:
                        info.bulk_.append(insert_one(data_doc.view()));
                        break;

                    case write_operation::Update:
                        info.bulk_.append(replace_one(pk_doc.view(), data_doc.view()));
                        break;

                    case write_operation::Revision:
                        info.bulk_.append(update_one(pk_doc.view(), make_document(kvp("$set", data_doc))));
                        break;

                    case write_operation::Remove:
                        info.bulk_.append(delete_one(pk_doc.view()));
                        break;

                    case write_operation::Unknown:
                        break;
                }
            }

            void execute_bulk(bulk_info_t_& info) {
                if (!info.op_cnt_) return;

                _detail::auto_reconnect([&]() { try {
                    info.bulk_.execute();
                } catch (const mongocxx::bulk_write_exception& e) {
                    error_ = e.what();
                    elog("Error on bulk write: ${code}, ${what}", ("what", error_)("code", e.code().value()));
                    if (_detail::get_mongo_code(e) != mongo_code::DuplicateValue) {
                        throw; // this shouldn't happen
                    }
                }});
            }
        }; // class write_ctx_t_

    }; // struct mongodb_driver::mongodb_impl_

    ///----

    mongodb_driver::mongodb_driver(journal& jrnl, string address, string sys_name)
    : impl_(new mongodb_impl_(jrnl, std::move(address), std::move(sys_name))) {
    }

    mongodb_driver::~mongodb_driver() = default;

    std::vector<table_def> mongodb_driver::db_tables(const account_name& code) const {
        return impl_->db_tables(code);
    }

    void mongodb_driver::create_index(const index_info& index) const {
        impl_->create_index(index);
    }

    void mongodb_driver::drop_index(const index_info& index) const {
        impl_->drop_index(index);
    }

    void mongodb_driver::drop_table(const table_info& table) const {
        impl_->drop_table(table);
    }

    void mongodb_driver::drop_db() {
        impl_->drop_db();
    }

    const cursor_info& mongodb_driver::clone(const cursor_request& request) {
        return impl_->clone_cursor(request);
    }

    void mongodb_driver::close(const cursor_request& request) {
        impl_->close_cursor(request);
    }

    void mongodb_driver::close_code_cursors(const account_name& code) {
        impl_->close_code_cursors(code);
    }

    void mongodb_driver::apply_code_changes(const account_name& code) {
        impl_->apply_code_changes(code);
    }

    void mongodb_driver::apply_all_changes() {
        impl_->apply_all_changes();
    }

    const cursor_info& mongodb_driver::lower_bound(index_info index, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(_detail::start_from(), std::move(key)).current();
        return cursor;
    }

    const cursor_info& mongodb_driver::upper_bound(index_info index, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(_detail::start_after(), std::move(key)).current();
        return cursor;
    }

    const cursor_info& mongodb_driver::find(index_info index, primary_key_t pk, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(_detail::start_from(), std::move(key), pk).current();
        return cursor;
    }

    const cursor_info& mongodb_driver::begin(index_info index) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open_begin().current();
        return cursor;
    }

    const cursor_info& mongodb_driver::end(index_info index) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open_end();
        return cursor;
    }

    const cursor_info& mongodb_driver::opt_find_by_pk(index_info index, primary_key_t pk, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open_by_pk(std::move(key), pk).current();
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

    const cursor_info& mongodb_driver::next(const cursor_info& info) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        cursor.next();
        return cursor;
    }

    const cursor_info& mongodb_driver::prev(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request).cursor();
        cursor.prev();
        return cursor;
    }

    const cursor_info& mongodb_driver::prev(const cursor_info& info) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        cursor.prev();
        return cursor;
    }

    primary_key_t mongodb_driver::available_pk(const table_info& table) {
        return impl_->available_pk(table);
    }

    object_value mongodb_driver::object_by_pk(const table_info& table, const primary_key_t pk) {
        return impl_->object_by_pk(table, pk);
    }

    const object_value& mongodb_driver::object_at_cursor(const cursor_info& info) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        return cursor.get_object_value();
    }

    void mongodb_driver::set_blob(const cursor_info& info, bytes blob) {
        auto& cursor = const_cast<mongodb_cursor_info&>(static_cast<const mongodb_cursor_info&>(info));
        cursor.blob = std::move(blob);
    }

} } // namespace cyberway::chaindb
