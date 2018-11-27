#pragma once

#include <cstdint>
#include <vector>

namespace bsoncxx { namespace types {
    struct b_decimal128;
    struct b_date;
    struct b_timestamp;
    struct b_binary;
    struct b_date;
}} // namespace bsoncxx::types

namespace fc {
    class time_point;
} // namespace fc

namespace cyberway { namespace chaindb {

    bsoncxx::types::b_decimal128 to_decimal128(uint64_t val);

    uint64_t from_decimal128(const bsoncxx::types::b_decimal128& val);

    fc::time_point from_date(const bsoncxx::types::b_date& date);

    fc::time_point from_timestamp(const bsoncxx::types::b_timestamp& timestamp);

    std::vector<char> build_blob_content(const bsoncxx::types::b_binary& src);

    bsoncxx::types::b_date to_date(const fc::time_point& date);

}} // namespace cyberway::chaindb
