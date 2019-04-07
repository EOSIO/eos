#include <limits>

#include <cyberway/chaindb/mongo_bigint_converter.hpp>
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
        Forward  =  1,
        Backward = -1
    }; // enum direction

    enum class mongo_code: int {
        Unknown        = -1,
        EmptyBulk      = 22,
        DuplicateValue = 11000,
        NoServer       = 13053
    }; // enum mongo_code

    namespace { namespace _detail {
        static const string pk_index_postfix = "_pk";
        static const string mongodb_id_index = "_id_";
        static const string mongodb_system   = "system.";

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

        bool is_asc_order(const string& name) {
            return name.size() == names::asc_order.size();
        }

        field_name get_order_field(const order_def& order) {
            field_name field = order.field;
            if (order.type == "uint128" || order.type == "int128") {
                field.append(1, '.').append(mongo_bigint_converter::BINARY_FIELD);
            }
            return field;
        }

        variant get_order_value(const variant_object& row, const index_info& index, const order_def& order) try {
            auto* object = &row;
            auto pos = order.path.size();
            for (auto& key: order.path) {
                auto itr = object->find(key);
                CYBERWAY_ASSERT(object->end() != itr, driver_absent_field_exception,
                    "Can't find the part ${key} for the field ${field} in the row ${row} from the table ${table}",
                    ("key", key)("field", order.field)("table", get_full_table_name(index))("row", row));

                --pos;
                if (0 == pos) {
                    return itr->value();
                } else {
                    object = &itr->value().get_object();
                }
            }
            CYBERWAY_ASSERT(false, driver_absent_field_exception,
                "Wrong logic on parsing of the field ${field} in the row ${row} from the table ${table}",
                ("table", get_full_table_name(index))("field", order.field)("row", row));
        } catch (const driver_absent_field_exception&) {
            throw;
        } catch (...) {
            CYBERWAY_ASSERT(false, driver_absent_field_exception,
                "External database can't read the field ${field} in the row ${row} from the table ${table}",
                ("field", order.field)("table", get_full_table_name(index))("row", row));
        }

        template <typename Lambda>
        void auto_reconnect(Lambda&& lambda) {
            // TODO: move to program options
            constexpr int max_iters = 12;
            constexpr int sleep_seconds = 5;

            for (int i = 0; i < max_iters; ++i) try {
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
            }

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

            if (source_) {
                // it is faster to get object from exist cursor then to open a new cursor, locate, and get object
                dst.object_    = get_object_value();
                dst.find_key_  = dst.object_.value;
                dst.find_pk_   = get_pk_value();
                // don't copy direction, because direction::Backward starts from previous, not from currently
                dst.direction_ = direction::Forward;
            } else {
                dst.find_key_  = find_key_;
                dst.find_pk_   = find_pk_;
                dst.object_    = object_;
                dst.direction_ = direction_;
            }

            dst.pk   = pk;
            dst.blob = blob;

            return dst;
        }

        // open() allows to reuse the same cursor for different cases
        mongodb_cursor_info& open(const direction dir, variant key, const primary_key_t locate_pk) {
            reset_object();
            source_.reset();

            pk         = locate_pk;
            direction_ = dir;

            find_pk_   = locate_pk;
            find_key_  = std::move(key);

            return *this;
        }

        void next() {
            if (direction::Backward == direction_) {
                // we at the last record of a range - we should get its key for correct locating
                lazy_open();
                change_direction(direction::Forward);
            }
            lazy_next();
        }

        void prev() {
            if (direction::Forward == direction_) {
                change_direction(direction::Backward);
                lazy_open();
            } else if (end_primary_key == pk) {
                lazy_open();
            } else {
                lazy_next();
            }
        }

        void current() {
            lazy_open();
        }

        const object_value& get_object_value() {
            lazy_open();
            if (!object_.value.is_null()) return object_;

            if (end_primary_key == get_pk_value()) {
                object_.clear();
                object_.service.pk    = pk;
                object_.service.code  = index.code;
                object_.service.scope = index.scope;
                object_.service.table = index.table->name;
            } else {
                auto& view = *source_->begin();
                object_ = build_object(index, view);
                pk      = object_.service.pk;
            }

            return object_;
        }

        bool is_openned() const {
            return !!source_;
        }

    private:
        collection    db_table_;

        direction     direction_ = direction::Forward;
        primary_key_t find_pk_   = unset_primary_key;
        variant       find_key_;

        std::unique_ptr<mongocxx::cursor> source_;
        object_value  object_;

        void change_direction(const direction dir) {
            direction_ = dir;
            if (source_) {
                find_key_ = get_object_value().value;
                find_pk_  = get_pk_value();
            }
            source_.reset();
        }

        void reset_object() {
            pk = unset_primary_key;

            if (!blob.empty())      blob.clear();
            if (!object_.is_null()) object_.clear();
        }

        document create_find_document() const {
            document find;
            find.append(kvp(names::scope_path, get_scope_name(index)));
            return find;
        }

        primary_key_t get_primary_pk_bound() const {
            if (find_pk_ > end_primary_key) {
                return find_pk_;
            }

            switch (index.pk_order->type.front()) {
                case 'i': // int64
                    if (direction::Forward == direction_) {
                        return std::numeric_limits<int64_t>::min();
                    } else {
                        return std::numeric_limits<int64_t>::max();
                    }
                case 'u': // uint64
                case 'n': // name
                case 's': // symbol_code
                    if (direction::Forward == direction_) {
                        return std::numeric_limits<uint64_t>::min();
                    } else {
                        return std::numeric_limits<uint64_t>::max();
                    }
                default:
                    CYBERWAY_ASSERT(false, driver_primary_key_exception,
                        "Invalid type ${type} for the primary key ${pk} in the table ${table}",
                        ("type", index.pk_order->type)("pk", pk)("table", get_full_table_name(index)));
            }
        }

        document create_bound_document() {
            document bound;

            if (!find_key_.is_object()) return bound;
            auto& find_object = find_key_.get_object();
            if (!find_object.size()) return bound;

            bound.append(kvp(names::scope_path, get_scope_name(index)));

            auto& orders = index.index->orders;
            for (auto& o: orders) {
                auto field = _detail::get_order_field(o);
                auto value = _detail::get_order_value(find_object, index, o);
                build_document(bound, field, value);
            }

            if (!index.index->unique) {
                append_pk_value(bound, index, get_primary_pk_bound());
            }

            return bound;
        }

        document create_sort_document() const {
            document sort;
            auto order = static_cast<int>(direction_);

            sort.append(kvp(names::scope_path, order));

            auto& orders = index.index->orders;
            for (auto& o: orders) {
                auto field = _detail::get_order_field(o);
                if (_detail::is_asc_order(o.order)) {
                    sort.append(kvp(field, order));
                } else {
                    sort.append(kvp(field, -order));
                }
            }

            if (!index.index->unique) {
                sort.append(kvp(index.pk_order->field, order));
            }

            return sort;
        }

        void lazy_open() {
            if (source_) return;

            reset_object();

            auto find  = create_find_document();
            auto bound = create_bound_document();
            auto sort  = create_sort_document();

            find_pk_ = unset_primary_key;

            auto opts = options::find()
                .hint(mongocxx::hint(db_name_to_string(index.index->name)))
                .sort(sort.view());

            if (direction_ == direction::Forward) {
                opts.min(bound.view());
            } else {
                opts.max(bound.view());
            }

            _detail::auto_reconnect([&]() {
                source_ = std::make_unique<mongocxx::cursor>(db_table_.find(find.view(), opts));
                init_pk_value();
            });
        }

        void lazy_next() {
            lazy_open();

            auto itr = source_->begin();
            if (source_->end() == itr) {
                return;
            }

            ++itr;
            if (source_->end() != itr || direction::Forward == direction_) {
                reset_object();
                init_pk_value();
            }
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

        mongodb_cursor_info& get_applied_cursor(cursor_info& info) {
            auto& cursor = static_cast<mongodb_cursor_info&>(info);
            if (!cursor.is_openned()) {
                apply_table_changes(cursor.index);
            }
            return cursor;
        }

        mongodb_cursor_info& get_applied_cursor(const cursor_info& info) {
            return get_applied_cursor(const_cast<cursor_info&>(info));
        }

        mongodb_cursor_info& get_applied_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            return get_applied_cursor(loc.cursor());
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

            code_cursor_map_.erase(itr);
        }

        std::vector<index_def> get_db_indexes(collection& db_table) const {
            std::vector<index_def> result;
            auto indexes = db_table.list_indexes();

            result.reserve(abi_info::max_index_cnt() * 2);
            for (auto& info: indexes) {
                index_def index;

                auto itr = info.find("name");
                if (info.end() == itr) continue;
                auto iname = itr->get_utf8().value;

                if (iname.ends_with(_detail::pk_index_postfix)) continue;
                if (!iname.compare( _detail::mongodb_id_index)) continue;

                try {
                    index.name = db_string_to_name(iname.data());
                } catch (const eosio::chain::name_type_exception&) {
                    db_table.indexes().drop_one(iname);
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
                        if (order.field.size() > mongo_bigint_converter::BINARY_FIELD.size() &&
                            key.ends_with(mongo_bigint_converter::BINARY_FIELD)
                        ) {
                            order.field.erase(order.field.size() - 1 - mongo_bigint_converter::BINARY_FIELD.size());
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

            tables.reserve(abi_info::max_table_cnt() * 2);
            _detail::auto_reconnect([&]() {
                tables.clear();
                auto db = mongo_conn_.database(get_code_name(sys_code_name_, code));
                auto names = db.list_collection_names();
                for (auto& tname: names) {
                    table_def table;

                    if (!tname.compare(0, _detail::mongodb_system.size(), _detail::mongodb_system)) {
                        continue;
                    }

                    try {
                        table.name = db_string_to_name(tname.c_str());
                    } catch (const eosio::chain::name_type_exception& e) {
                        db.collection(tname).drop();
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
                auto field = _detail::get_order_field(order);
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

        template <typename... Args>
        mongodb_cursor_info& create_cursor(index_info index, Args&&... args) {
            auto code = index.code;
            auto itr = code_cursor_map_.find(code);
            auto id = get_next_cursor_id(itr);
            auto db_table = get_db_table(index);
            mongodb_cursor_info new_cursor(id, std::move(index), std::move(db_table), std::forward<Args>(args)...);
            // apply_table_changes(index);
            return add_cursor(std::move(itr), code, std::move(new_cursor));
        }

        mongodb_cursor_info& clone_cursor(const cursor_request& request) {
            auto loc = get_cursor(request);
            auto next_id = get_next_cursor_id(loc.code_itr_);

            auto cloned_cursor = loc.cursor().clone(next_id);
            return add_cursor(loc.code_itr_, request.code, std::move(cloned_cursor));
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
                obj.value = variant();
                return obj;
            }

            CYBERWAY_ASSERT(false, driver_absent_object_exception,
                "External database doesn't contain object with the primary key ${pk} in the table ${table}",
                ("pk", pk)("table", get_full_table_name(table)));
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

    cursor_info& mongodb_driver::lower_bound(index_info index, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(direction::Forward, std::move(key), unset_primary_key);
        return cursor;
    }

    cursor_info& mongodb_driver::upper_bound(index_info index, variant key) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(direction::Backward, std::move(key), unset_primary_key).next();
        return cursor;
    }

    cursor_info& mongodb_driver::locate_to(index_info index, variant key, primary_key_t pk) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(direction::Forward, std::move(key), pk);
        return cursor;
    }

    cursor_info& mongodb_driver::begin(index_info index) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(direction::Forward, {}, unset_primary_key);
        return cursor;
    }

    cursor_info& mongodb_driver::end(index_info index) {
        auto& cursor = impl_->create_cursor(std::move(index));
        cursor.open(direction::Backward, {}, end_primary_key);
        return cursor;
    }

    cursor_info& mongodb_driver::current(const cursor_info& info) {
        auto& cursor = impl_->get_applied_cursor(info);
        cursor.current();
        return cursor;
    }

    cursor_info& mongodb_driver::current(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request);
        cursor.current();
        return cursor;
    }

    cursor_info& mongodb_driver::next(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request);
        cursor.next();
        return cursor;
    }

    cursor_info& mongodb_driver::next(const cursor_info& info) {
        auto& cursor = impl_->get_applied_cursor(info);
        cursor.next();
        return cursor;
    }

    cursor_info& mongodb_driver::prev(const cursor_request& request) {
        auto& cursor = impl_->get_applied_cursor(request);
        cursor.prev();
        return cursor;
    }

    cursor_info& mongodb_driver::prev(const cursor_info& info) {
        auto& cursor = impl_->get_applied_cursor(info);
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
        auto& cursor = impl_->get_applied_cursor(info);
        return cursor.get_object_value();
    }

    void mongodb_driver::set_blob(const cursor_info& info, bytes blob) {
        auto& cursor = impl_->get_applied_cursor(info);
        cursor.blob = std::move(blob);
    }

} } // namespace cyberway::chaindb
