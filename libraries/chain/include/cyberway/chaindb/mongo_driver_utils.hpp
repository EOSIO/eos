#pragma once

#include <cstdint>
#include <vector>

#include <bsoncxx/config/prelude.hpp>

namespace bsoncxx {
BSONCXX_INLINE_NAMESPACE_BEGIN
    namespace types {
        struct b_decimal128;
        struct b_date;
        struct b_timestamp;
        struct b_binary;
        struct b_date;
    } // namespace types

    namespace document {
        class view;
    } // namespace document

    namespace builder { namespace basic {
        class sub_document;
    } } // namespace builder::basic
BSONCXX_INLINE_NAMESPACE_END
} // namespace bsoncxx

namespace  fc {
    class variant;
    class variant_object;
    class time_point;
} // namespace fc

namespace cyberway { namespace chaindb {

    struct table_info;
    struct object_value;

    using primary_key_t  = uint64_t;
    using account_name_t = uint64_t;

    namespace basic = bsoncxx::builder::basic;
    namespace types = bsoncxx::types;

    std::string to_json(const bsoncxx::document::view&);

    object_value build_object(const table_info&, const bsoncxx::document::view&);

    basic::sub_document& append_pk_value(basic::sub_document&, const table_info&, primary_key_t);

    basic::sub_document& build_document(basic::sub_document&, const object_value&);
    basic::sub_document& build_document(basic::sub_document&, const std::string&, const fc::variant&);
    basic::sub_document& build_bound_document(basic::sub_document&, const std::string&, int);
    basic::sub_document& build_service_document(basic::sub_document&, const table_info&, const object_value&);
    basic::sub_document& build_undo_service_document(basic::sub_document&, const table_info&, const object_value&);
    basic::sub_document& build_find_pk_document(basic::sub_document&, const table_info&, const object_value&);
    basic::sub_document& build_find_undo_pk_document(basic::sub_document&, const table_info&, const object_value&);

    types::b_decimal128 to_decimal128(uint64_t val);

    uint64_t from_decimal128(const types::b_decimal128& val);

    fc::time_point from_date(const types::b_date& date);

    fc::time_point from_timestamp(const types::b_timestamp& timestamp);

    std::vector<char> build_blob_content(const types::b_binary& src);

    types::b_date to_date(const fc::time_point& date);

    primary_key_t get_pk_value(const table_info&, const bsoncxx::document::view&);
    account_name_t get_scope_value(const table_info&, const bsoncxx::document::view&);

}} // namespace cyberway::chaindb
