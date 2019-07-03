#include <chrono>
#include <iomanip>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/mongo_driver_utils.hpp>
#include <cyberway/chaindb/mongo_bigint_converter.hpp>
#include <cyberway/chaindb/mongo_asset_converter.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/noscope_tables.hpp>

#include <fc/time.hpp>
#include <fc/variant_object.hpp>

#include <bsoncxx/json.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <eosio/chain/asset.hpp>

namespace {
    constexpr uint64_t DECIMAL_128_BIAS = 6176;
    constexpr uint64_t DECIMAL_128_HIGH = DECIMAL_128_BIAS << 49;
}

namespace cyberway { namespace chaindb {

    using eosio::chain::name;
    using eosio::chain::symbol;
    using eosio::chain::symbol_info;

    using fc::__uint128;
    using fc::variants;
    using fc::variant_object;
    using fc::mutable_variant_object;
    using fc::time_point;

    using bsoncxx::types::b_null;
    using bsoncxx::types::b_minkey;
    using bsoncxx::types::b_maxkey;
    using bsoncxx::types::b_oid;
    using bsoncxx::types::b_bool;
    using bsoncxx::types::b_double;
    using bsoncxx::types::b_int64;
    using bsoncxx::types::b_date;
    using bsoncxx::types::b_decimal128;
    using bsoncxx::types::b_binary;
    using bsoncxx::types::b_array;
    using bsoncxx::types::b_timestamp;
    using bsoncxx::types::b_document;

    using bsoncxx::builder::basic::sub_array;
    using bsoncxx::builder::basic::sub_document;
    using bsoncxx::builder::basic::document;
    using bsoncxx::builder::basic::kvp;

    using bsoncxx::document::element;
    using document_view = bsoncxx::document::view;
    using array_view = bsoncxx::array::view;

    using bsoncxx::type;
    using bsoncxx::binary_sub_type;
    using bsoncxx::oid;



    string to_json(const document_view& row) try {
        auto s = bsoncxx::to_json(row);
        if (s.empty()) s = "?";
        return s;
    } catch(...) {
        return string("?");
    }

    bsoncxx::types::b_decimal128 to_decimal128(uint64_t val) {
        bsoncxx::decimal128 decimal = {DECIMAL_128_HIGH, val};
        return bsoncxx::types::b_decimal128(decimal);
    }

    string from_utf8(const element& elem) {
        return elem.get_utf8().value.to_string();
    }

    uint64_t from_decimal128(const bsoncxx::types::b_decimal128& val) {
        return val.value.low();
    }

    uint64_t from_decimal128(const element& elem) {
        return from_decimal128(elem.get_decimal128());
    }

    fc::time_point from_date(const bsoncxx::types::b_date& date) {
        std::chrono::system_clock::time_point tp = date;
        return fc::time_point(fc::seconds(std::chrono::system_clock::to_time_t(tp)));
    }

    time_point from_timestamp(const b_timestamp& timestamp) {
        return time_point(fc::milliseconds(timestamp.timestamp * 1000000));
    }

    std::vector<char> build_blob_content(const b_binary& src) {
        std::vector<char> blob_content;
        if (src.sub_type != bsoncxx::binary_sub_type::k_binary) return blob_content;

        blob_content.reserve(src.size);
        blob_content.assign(src.bytes, src.bytes + src.size);
        return blob_content;
    }

    bsoncxx::types::b_date to_date(const fc::time_point& date) {
        const auto as_time_point = std::chrono::system_clock::from_time_t(date.sec_since_epoch());
        return bsoncxx::types::b_date(as_time_point);
    }

    variant build_variant(const document_view&, bool);

    variants build_variant(const array_view& src, const bool with_decors) {
        variants dst;

        dst.reserve(std::distance(src.begin(), src.end()));
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
                    dst.emplace_back(from_decimal128(item.get_decimal128()));
                    break;
                case type::k_double:
                    dst.emplace_back(item.get_double().value);
                    break;
                case type::k_utf8:
                    dst.emplace_back(item.get_utf8().value.to_string());
                    break;
                case type::k_date:
                    dst.emplace_back(from_date(item.get_date()));
                    break;
                case type::k_timestamp:
                    dst.emplace_back(from_timestamp(item.get_timestamp()));
                    break;
                case type::k_document:
                    dst.emplace_back(build_variant(item.get_document().value, with_decors));
                    break;
                case type::k_array:
                    dst.emplace_back(build_variant(item.get_array().value, with_decors));
                    break;
                case type::k_binary:
                    dst.emplace_back(blob{build_blob_content(item.get_binary())});
                    break;
                case type::k_bool:
                    dst.emplace_back(item.get_bool().value);
                    break;

                // SKIP
                case type::k_oid:
                    break;

                case type::k_code:
                case type::k_codewscope:
                case type::k_symbol:
                case type::k_dbpointer:
                case type::k_regex:
                case type::k_maxkey:
                case type::k_minkey:
                case type::k_undefined:
                default:
                    CYBERWAY_THROW(driver_wrong_field_type_exception, "Bad type of the database field");
                    break;
            }
        }
        return dst;
    }

    template <typename Result> Result get_typed_value(const table_info& info, const element& elem) {
        switch (elem.type()) {
            case type::k_int64:
                return Result::from_value(info, elem.get_int64().value);
            case type::k_decimal128:
                return Result::from_value(info, from_decimal128(elem));
            case type::k_utf8:
                return Result::from_value(info, from_utf8(elem));
            case type::k_document: {
                const mongo_asset_converter converter(elem.get_document().value);
                if (converter.is_valid_value()) {
                    return Result::from_value(info, converter.value());
                }
            }
            default:
                break;
        }
        return Result::from_value(info);
    }

    primary_key_t get_pk_value(const table_info& table, const document_view& row) try {
        document_view view = row;
        auto& pk_order = *table.pk_order;
        auto pos = pk_order.path.size();
        for (auto& key: pk_order.path) {
            auto itr = view.find(key);
            CYBERWAY_ASSERT(view.end() != itr, driver_primary_key_exception,
                "Can't find the part ${key} for the primary key ${pk} in the row '${row}' from the table ${table}",
                ("key", key)("pk", pk_order.field)("row", to_json(row))("table", get_full_table_name(table)));

            --pos;
            if (0 == pos) {
                return get_typed_value<primary_key>(table, *itr).value();
            } else {
                view = itr->get_document().value;
            }
        }
        CYBERWAY_THROW(driver_primary_key_exception,
            "Wrong logic on parsing of the primary key ${pk} in the row '${row}' from the table ${table}",
            ("pk", pk_order.field)("row", to_json(row))("table", get_full_table_name(table)));
    } catch(const driver_primary_key_exception&) {
        throw;
    } catch (...) {
        CYBERWAY_THROW(driver_primary_key_exception,
            "External database can't read the primary key ${pk} in the row '${row}' from the table ${table}",
            ("pk", table.pk_order->field)("row", to_json(row))("table", get_full_table_name(table)));
    }

    scope_name_t get_scope_value(const table_info& table, const bsoncxx::document::view& row) try {
        auto service_itr = row.find(names::service_field);
        auto scope_itr = service_itr->get_document().value.find(names::scope_field);
        return get_typed_value<scope_name>(table, *scope_itr).value();
    } catch (...) {
        CYBERWAY_THROW(driver_scope_exception,
            "External database can't read the scope in the row '${row}' from the table ${table}",
            ("row", to_json(row))("table", get_full_table_name(table)));
    }

    static inline void validate_field_name(const bool test, const document_view& doc, const element& itm) {
        CYBERWAY_ASSERT(test, driver_wrong_field_name_exception,
            "Wrong field name ${name} in the row '${row}",
            ("name", itm.key().to_string())("row", to_json(doc)));
    }

    static inline void validate_field_type(const bool test, const document_view& doc, const element& itm) {
        CYBERWAY_ASSERT(test, driver_wrong_field_type_exception,
            "Wrong type for the field ${name} in the document '${row}",
            ("name", itm.key().to_string())("row", to_json(doc)));
    }

    static inline void validate_exist_field(const bool test, const document_view& doc) {
        CYBERWAY_ASSERT(test, driver_absent_field_exception,
            "The row ${row} doesn't contain the some _SERVICE_ fields", ("row", to_json(doc)));
    }

    void build_undo_state(const table_info& info, service_state& state, const document_view& src) {
        int field_cnt = 0;
        for (auto& itm: src) try {
            ++field_cnt;
            auto key = itm.key().to_string();
            switch (key.front()) {
                case 'c': {
                    validate_field_name(names::code_field == key, src, itm);
                    state.code = from_decimal128(itm);
                    break;
                }
                case 't': {
                    validate_field_name(names::table_field == key, src, itm);
                    state.table = from_decimal128(itm);
                    break;
                }
                case 's': {
                    if (names::scope_field == key) {
                        state.scope = from_decimal128(itm);
                    } else {
                        validate_field_name(names::size_field == key, src, itm);
                        state.size = itm.get_int32().value;
                    }
                    break;
                }
                case 'p': {
                    if (key == names::pk_field) {
                        state.pk = from_decimal128(itm);
                    } else {
                        validate_field_name(names::payer_field == key, src, itm);
                        state.payer = from_decimal128(itm);
                    }
                    break;
                }
                case 'r': {
                    if (key == names::revision_field) {
                        state.revision = itm.get_int64().value;
                    } else {
                        validate_field_name(names::ram_field == key, src, itm);
                        --field_cnt; // skip to count it
                        state.in_ram = itm.get_bool().value;
                        // for archive records it should absent
                        validate_field_name(state.in_ram, src, itm);
                    }
                    break;
                }
                case 'u': {
                    switch (key[1]) {
                        case 'p': {
                            validate_field_name(names::undo_pk_field == key, src, itm);
                            state.undo_pk = from_decimal128(itm);
                            break;
                        }
                        case 'y': {
                            validate_field_name(names::undo_payer_field == key, src, itm);
                            state.undo_payer = from_decimal128(itm);
                            break;
                        }
                        case 's': {
                            validate_field_name(names::undo_size_field == key, src, itm);
                            state.undo_size = itm.get_int32().value;
                            break;
                        }
                        case 'c': {
                            validate_field_name(names::undo_rec_field == key, src, itm);
                            state.undo_rec = static_cast<undo_record>(itm.get_int32().value);
                            break;
                        }
                        default: {
                            validate_field_name(names::undo_ram_field == key, src, itm);
                            --field_cnt; // skip to count it
                            state.undo_in_ram = itm.get_bool().value;
                            // for archive records it should absent
                            validate_field_name(state.undo_in_ram, src, itm);
                            break;
                        }
                    }
                    break;
                }
                default: {
                    validate_field_name(false, src, itm);
                }
            }
        } catch(const driver_wrong_field_name_exception&) {
            throw;
        } catch(const driver_wrong_field_type_exception&) {
            throw;
        } catch(...) {
            validate_field_type(false, src, itm);
        }

        constexpr int undo_field_cnt = 13 /* list of them above */ - 2 /* ram_field */;
        validate_exist_field(undo_field_cnt == field_cnt, src);
    }

    void build_service_state(const table_info& info, service_state& state, const document_view& src) {
        // The number of RAM objects is less then the number of archive objects.
        //   so, the RAM field is false by default
        state.in_ram      = false;
        state.undo_in_ram = false;

        if (src.begin()->key().front() == 'u') {
            build_undo_state(info, state, src);
            return;
        }

        int field_cnt = 0;
        for (auto& itm: src) try {
            auto key = itm.key().to_string();
            ++field_cnt;
            switch (key.front()) {
                case 's': {
                    if (names::scope_field == key) {
                        state.scope = get_typed_value<scope_name>(info, itm).value();
                    } else {
                        validate_field_name(names::size_field == key, src, itm);
                        state.size = itm.get_int32().value;
                    }
                    break;
                }
                case 'p': {
                    validate_field_name(names::payer_field == key, src, itm);
                    state.payer = account_name(from_utf8(itm));
                    break;
                }
                case 'r': {
                    if (key == names::revision_field) {
                        state.revision = itm.get_int64().value;
                    } else {
                        validate_field_name(names::ram_field == key, src, itm);
                        --field_cnt; // skip to count it
                        state.in_ram = itm.get_bool().value;
                        // for archive records it should absent
                        validate_field_type(state.in_ram, src, itm);
                    }
                    break;
                }
                default: {
                    validate_field_name(false, src, itm);
                }
            }
        } catch(const driver_wrong_field_name_exception&) {
            throw;
        } catch(const driver_wrong_field_type_exception&) {
            throw;
        } catch(...) {
            validate_field_type(false, src, itm);
        }

        constexpr int service_field_cnt = 5 /* list of them above */ - 1 /* ram_field */;
        validate_exist_field(service_field_cnt == field_cnt, src);
    }

    void build_variant(mutable_variant_object& dst, string key, const element& src, const bool with_decors) {
        switch (src.type()) {
            case type::k_null:
                dst(std::move(key), variant());
                break;
            case type::k_int32:
                dst(std::move(key), src.get_int32().value);
                break;
            case type::k_int64:
                dst(std::move(key), src.get_int64().value);
                break;
            case type::k_decimal128:
                dst(std::move(key), from_decimal128(src));
                break;
            case type::k_double:
                dst(std::move(key), src.get_double().value);
                break;
            case type::k_utf8:
                dst(std::move(key), from_utf8(src));
                break;
            case type::k_date:
                dst(std::move(key), from_date(src.get_date()));
                break;
            case type::k_timestamp:
                dst(std::move(key), from_timestamp(src.get_timestamp()));
                break;
            case type::k_document:
                dst(std::move(key), build_variant(src.get_document().value, with_decors));
                break;
            case type::k_array:
                dst(std::move(key), build_variant(src.get_array().value, with_decors));
                break;
            case type::k_binary:
                dst(std::move(key), blob{build_blob_content(src.get_binary())});
                break;
            case type::k_bool:
                dst(std::move(key), src.get_bool().value);
                break;

            // SKIP
            case type::k_oid:
                break;

            case type::k_code:
            case type::k_codewscope:
            case type::k_symbol:
            case type::k_dbpointer:
            case type::k_regex:
            case type::k_maxkey:
            case type::k_minkey:
            case type::k_undefined:
            default:
                CYBERWAY_THROW(driver_wrong_field_type_exception, "Bad type of the database field");
                break;
        }
    }

    variant build_variant(const document_view& src, const bool with_decors) {
        const mongo_bigint_converter bigint_converter(src);
        if (bigint_converter.is_valid_value()) {
            return bigint_converter.get_raw_value();
        }

        if (with_decors) {
            const mongo_asset_converter asset_converter(src);
            if (asset_converter.is_valid_value()) {
                return asset_converter.get_variant();
            }
        }

        mutable_variant_object dst;
        for (auto& itm: src) {
            build_variant(dst, itm.key().to_string(), itm, with_decors);
        }
        return variant(std::move(dst));
    }

    object_value build_object(const table_info& info, const document_view& src, const bool with_decors) {
        object_value obj;
        bool was_service = false;
        mutable_variant_object dst;

        for (auto& itm: src) {
            auto key = itm.key().to_string();
            if ('_' == key.front()) {
                if (key == "_id") continue;

                validate_field_name(!was_service && names::service_field == key, src, itm);
                build_service_state(info, obj.service, itm.get_document().value);
                was_service = true;
            } else {
                build_variant(dst, std::move(key), itm, with_decors);
            }
        }
        validate_exist_field(was_service, src);

        obj.value = variant(std::move(dst));
        if (obj.pk() == primary_key::Unset) {
            obj.service.pk    = get_pk_value(info, src);
            obj.service.code  = info.code;
            obj.service.scope = info.scope;
            obj.service.table = info.table_name();
        }
        return obj;
    }

    b_binary build_binary(const blob& src) {
        auto size = uint32_t(src.data.size());
        auto data = reinterpret_cast<const uint8_t*>(src.data.data());
        return b_binary{binary_sub_type::k_binary, size, data};
    }

    sub_document& build_bigint(sub_document& dst, const mongo_bigint_converter& src) {
        blob binary;
        binary.data = src.get_blob_value();
        dst.append(kvp(src.BINARY_FIELD, build_binary(binary)));
        dst.append(kvp(src.STRING_FIELD, src.get_string_value()));
        return dst;
    }

    sub_document& build_document(sub_document&, const variant_object&);

    sub_array& build_document(sub_array& dst, const variants& src) {
        for (auto& item: src) {
            switch (item.get_type()) {
                case variant::type_id::null_type:
                    dst.append(b_null());
                    break;
                case variant::type_id::int64_type:
                    dst.append(b_int64{item.as_int64()});
                    break;
                case variant::type_id::uint64_type:
                    dst.append(to_decimal128(item.as_uint64()));
                    break;
                case variant::type_id::int128_type:
                    dst.append([&](sub_document sub_doc){
                        build_bigint(sub_doc, mongo_bigint_converter(item.as_int128()));
                    });
                    break;
                case variant::type_id::uint128_type:
                    dst.append([&](sub_document sub_doc){
                        build_bigint(sub_doc, mongo_bigint_converter(item.as_uint128()));
                    });
                    break;
                case variant::type_id::double_type:
                    dst.append(b_double{item.as_double()});
                    break;
                case variant::type_id::bool_type:
                    dst.append(b_bool{item.as_bool()});
                    break;
                case variant::type_id::string_type:
                    dst.append(item.as_string());
                    break;
                case variant::type_id::time_type:
                    dst.append(item.as_time_point());
                    break;
                case variant::type_id::array_type:
                    dst.append([&](sub_array array){ build_document(array, item.get_array()); });
                    break;
                case variant::type_id::object_type:
                    dst.append([&](sub_document sub_doc){ build_document(sub_doc, item.get_object()); });
                    break;
                case variant::type_id::blob_type:
                    // dst.append(build_binary(item.as_blob()));
                    dst.append(item.as_string());
                    break;
            }
        }
        return dst;
    }

    template<typename BigIntBuilder>
    sub_document& build_document(sub_document& dst, const string& key, const variant& src, BigIntBuilder&& bigint) {
        switch (src.get_type()) {
            case variant::type_id::null_type:
                dst.append(kvp(key, b_null()));
                break;
            case variant::type_id::int64_type:
                dst.append(kvp(key, b_int64{src.as_int64()}));
                break;
            case variant::type_id::uint64_type:
                dst.append(kvp(key, to_decimal128(src.as_uint64())));
                break;
            case variant::type_id::int128_type:
                bigint.build(dst, key, src.as_int128());
                break;
            case variant::type_id::uint128_type:
                bigint.build(dst, key, src.as_uint128());
                break;
            case variant::type_id::double_type:
                dst.append(kvp(key, b_double{src.as_double()}));
                break;
            case variant::type_id::bool_type:
                dst.append(kvp(key, b_bool{src.as_bool()}));
                break;
            case variant::type_id::string_type:
                dst.append(kvp(key, src.as_string()));
                break;
            case variant::type_id::time_type:
                dst.append(kvp(key, src.as_time_point()));
                break;
            case variant::type_id::array_type:
                dst.append(kvp(key, [&](sub_array array){ build_document(array, src.get_array()); }));
                break;
            case variant::type_id::object_type:
                dst.append(kvp(key, [&](sub_document sub_doc){ build_document(sub_doc, src.get_object()); }));
                break;
            case variant::type_id::blob_type:
                // dst.append(kvp(key, build_binary(src.as_blob())));
                dst.append(kvp(key, src.as_string()));
                break;
        }
        return dst;
    }

    struct bigint_value final {
        template<typename Value> void build(sub_document& dst, const string& key, const Value& value) const {
            auto binary = blob{mongo_bigint_converter(value).get_blob_value()};
            dst.append(kvp(key, build_binary(binary)));
        }
    }; // struct bigint_value

    sub_document& build_document(sub_document& dst, const string& key, const variant& src) {
        return build_document(dst, key, src, bigint_value());
    }

    sub_document& build_bound_document(basic::sub_document& dst, const std::string& key, const int order) {
        switch (order) {
            case 1:
                dst.append(kvp(key, b_minkey()));
                break;
            case -1:
                dst.append(kvp(key, b_maxkey()));
                break;

            default:
                assert(false);
        }
        return dst;
    }

    struct bigint_subdocument final {
        template<typename Value> void build(sub_document& dst, const string& key, const Value& value) const {
            dst.append(kvp(key, [&](sub_document sub_doc){
                build_bigint(sub_doc, mongo_bigint_converter(value));
            }));
        }
    }; // struct bigint_subdocument

    sub_document& build_document(sub_document& dst, const variant_object& src) {
        for (auto& item: src) {
            build_document(dst, item.key(), item.value(), bigint_subdocument());
        }
        return dst;
    }

    sub_document& build_document(sub_document& dst, const object_value& obj) {
        if (obj.is_null()) {
            return dst;
        }
        return build_document(dst, obj.value.get_object());
    }

    static inline void append_ram_field(sub_document& doc, const string& name, const bool value) {
        if (value) {
            // for archive records it should absent
            doc.append(kvp(name, true));
        }
    }

    void append_typed_value(sub_document& doc, const field_name& field, const typed_name& typed) {
        switch (typed.kind()) {
            case typed_name::Int64:
                doc.append(kvp(field, static_cast<int64_t>(typed.value())));
                break;
            case typed_name::Uint64:
                doc.append(kvp(field, to_decimal128(typed.value())));
                break;
            case typed_name::Symbol: // should not happen for the primary key ..
            case typed_name::Name:
            case typed_name::SymbolCode:
                doc.append(kvp(field, typed.to_string()));
                break;
            case typed_name::Unknown:
            default:
                assert(false);
                break;
        }
    }

    sub_document& build_service_document(sub_document& doc, const table_info& table, const object_value& obj) {
        assert(obj.service.scope == table.scope);
        doc.append(kvp(names::service_field, [&](sub_document serv_doc) {
            append_typed_value(serv_doc, names::scope_field, scope_name::from_table(table));
            serv_doc.append(kvp(names::revision_field, obj.service.revision));
            serv_doc.append(kvp(names::payer_field, get_payer_name(obj.service.payer)));
            serv_doc.append(kvp(names::size_field, static_cast<int32_t>(obj.service.size)));
            append_ram_field(serv_doc, names::ram_field, obj.service.in_ram);
        }));
        return doc;
    }

    sub_document& build_undo_document(sub_document& doc, const table_info& table, const object_value& obj) {
        assert(obj.service.scope == table.scope);
        doc.append(kvp(names::service_field, [&](sub_document serv_doc) {
            serv_doc.append(kvp(names::undo_pk_field, to_decimal128(obj.service.undo_pk)));
            serv_doc.append(kvp(names::undo_rec_field, static_cast<int32_t>(obj.service.undo_rec)));
            serv_doc.append(kvp(names::undo_payer_field, to_decimal128(obj.service.undo_payer)));
            serv_doc.append(kvp(names::undo_size_field, static_cast<int32_t>(obj.service.undo_size)));
            serv_doc.append(kvp(names::code_field, to_decimal128(obj.service.code)));
            serv_doc.append(kvp(names::table_field, to_decimal128(obj.service.table)));
            serv_doc.append(kvp(names::scope_field, to_decimal128(obj.service.scope)));
            serv_doc.append(kvp(names::pk_field, to_decimal128(obj.service.pk)));
            serv_doc.append(kvp(names::revision_field, obj.service.revision));
            serv_doc.append(kvp(names::payer_field, to_decimal128(obj.service.payer)));
            serv_doc.append(kvp(names::size_field, static_cast<int32_t>(obj.service.size)));
            append_ram_field(serv_doc, names::ram_field, obj.service.in_ram);
            append_ram_field(serv_doc, names::undo_ram_field, obj.service.undo_in_ram);
        }));
        return doc;
    }

    sub_document& append_pk_value(sub_document& doc, const table_info& table, const primary_key_t pk) {
        assert(table.pk_order);

        auto field = table.pk_order->field;
        auto typed = primary_key::from_table(table, pk);

        if (typed_name::Symbol == typed.kind()) {
            // The driver gets objects as variant in which the (symbol) primary key is already exist as symbol_info
            //     for such cases the driver should interpret the (symbol) primary key as structure
            doc.append(kvp(field, [&](sub_document sub_doc) {
                auto symb = symbol(typed.value());
                sub_doc.append(kvp(names::decs_field, to_decimal128(symb.decimals())));
                sub_doc.append(kvp(names::sym_field,  symb.name()));
            }));
        } else {
            append_typed_value(doc, field, typed);
        }
        return doc;
    }

    sub_document& append_scope_value(sub_document& doc, const table_info& table) {
        append_typed_value(doc, names::scope_path, scope_name::from_table(table));
        return doc;
    }

    sub_document& build_find_pk_document(sub_document& doc, const table_info& table, const object_value& obj) {
        if (!is_noscope_table(table)) {
            append_scope_value(doc, table);
        }
        append_pk_value(doc, table, obj.service.pk);
        return doc;
    }

    sub_document& build_find_undo_pk_document(sub_document& doc, const table_info&, const object_value& obj) {
        doc.append(kvp(names::undo_pk_path, to_decimal128(obj.service.undo_pk)));
        return doc;
    }

} } // namespace cyberway::chaindb
