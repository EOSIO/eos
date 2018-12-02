#include <chrono>
#include <iomanip>

#include <cyberway/chaindb/mongo_driver_utils.hpp>
#include <cyberway/chaindb/mongo_big_int_converter.hpp>

#include <fc/time.hpp>
#include <fc/variant_object.hpp>

#include <bsoncxx/types.hpp>

#include <bsoncxx/document/view.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace {
    constexpr uint64_t DECIMAL_128_BIAS = 6176;
    constexpr uint64_t DECIMAL_128_HIGH = DECIMAL_128_BIAS << 49;
}

namespace cyberway { namespace chaindb {

    using std::string;

    using fc::blob;
    using fc::__uint128;
    using fc::variant;
    using fc::variants;
    using fc::variant_object;
    using fc::mutable_variant_object;
    using fc::time_point;

    using bsoncxx::types::b_null;
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
    using bsoncxx::builder::basic::kvp;

    using bsoncxx::document::element;
    using document_view = bsoncxx::document::view;
    using array_view = bsoncxx::array::view;

    using bsoncxx::type;
    using bsoncxx::binary_sub_type;
    using bsoncxx::oid;

    bsoncxx::types::b_decimal128 to_decimal128(uint64_t val) {
        bsoncxx::decimal128 decimal = {DECIMAL_128_HIGH, val};
        return bsoncxx::types::b_decimal128(decimal);
    }

    uint64_t from_decimal128(const bsoncxx::types::b_decimal128& val) {
        return val.value.low();
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
        if (src.sub_type != bsoncxx::binary_sub_type::k_binary) return {};

        blob_content.assign(src.bytes, src.bytes + src.size);
        return blob_content;
    }
    bsoncxx::types::b_date to_date(const fc::time_point& date) {
        const auto as_time_point = std::chrono::system_clock::from_time_t(date.sec_since_epoch());
        return bsoncxx::types::b_date(as_time_point);
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
                    dst.emplace_back(build_variant(item.get_document().value));
                    break;
                case type::k_array:
                    dst.emplace_back(build_variant(item.get_array().value));
                    break;
                case type::k_binary:
                    dst.emplace_back(blob{build_blob_content(item.get_binary())});
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
                dst.set(std::move(key), from_decimal128(src.get_decimal128()));
                break;
            case type::k_double:
                dst.set(std::move(key), src.get_double().value);
                break;
            case type::k_utf8:
                dst.set(std::move(key), src.get_utf8().value.to_string());
                break;
            case type::k_date:
                dst.set(std::move(key), from_date(src.get_date()));
                break;
            case type::k_timestamp:
                dst.set(std::move(key), from_timestamp(src.get_timestamp()));
                break;
            case type::k_document:
                dst.set(std::move(key), build_variant(src.get_document().value));
                break;
            case type::k_array:
                dst.set(std::move(key), build_variant(src.get_array().value));
                break;
            case type::k_binary:
                dst.set(std::move(key), blob{build_blob_content(src.get_binary())});
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
        const mongo_big_int_converter converter(src);
        if (converter.is_valid_value()) {
            return converter.get_raw_value();
        }

        mutable_variant_object dst;
        for (auto& item: src) {
            build_variant(dst, item.key().to_string(), item);
        }
        return variant(std::move(dst));
    }

    b_binary build_binary(const blob& src) {
        auto size = uint32_t(src.data.size());
        auto data = reinterpret_cast<const uint8_t*>(src.data.data());
        return b_binary{binary_sub_type::k_binary, size, data};
    }

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
                        build_document(sub_doc, mongo_big_int_converter(item.as_int128()).as_object_encoded());
                    });
                    break;
                case variant::type_id::uint128_type:
                    dst.append([&](sub_document sub_doc){
                        build_document(sub_doc, mongo_big_int_converter(item.as_uint128()).as_object_encoded());
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
                    dst.append(build_binary(item.as_blob()));
                    break;
            }
        }
        return dst;
    }

    sub_document& build_document(sub_document& dst, const string& key, const variant& src) {
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
                dst.append(kvp(key, [&](sub_document sub_doc){
                    build_document(sub_doc, mongo_big_int_converter(src.as_int128()).as_object_encoded());
                } ));
                break;
            case variant::type_id::uint128_type:
                dst.append(kvp(key, [&](sub_document sub_doc){
                    build_document(sub_doc, mongo_big_int_converter(src.as_uint128()).as_object_encoded());
                } ));
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

}} // namespace cyberway::chaindb
