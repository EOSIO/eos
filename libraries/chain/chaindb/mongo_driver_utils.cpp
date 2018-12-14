#include <chrono>
#include <iomanip>

#include <bsoncxx/types.hpp>

#include "fc/time.hpp"
#include "fc/variant_object.hpp"

#include "cyberway/chaindb/mongo_driver_utils.h"

namespace {
    constexpr uint64_t DECIMAL_128_BIAS = 6176;
    constexpr uint64_t DECIMAL_128_HIGH = DECIMAL_128_BIAS << 49;
}

namespace cyberway { namespace chaindb {

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

    fc::time_point from_timestamp(const bsoncxx::types::b_timestamp& timestamp) {
        return fc::time_point(fc::milliseconds(timestamp.timestamp * 1000000));
    }

    std::vector<char> build_blob_content(const bsoncxx::types::b_binary& src) {
        std::vector<char> blob_content;
        if (src.sub_type != bsoncxx::binary_sub_type::k_binary) return {};
        blob_content.assign(src.bytes, src.bytes + src.size);
        return blob_content;
    }

    bsoncxx::types::b_date to_date(const fc::time_point& date) {
        const auto as_time_point = std::chrono::system_clock::from_time_t(date.sec_since_epoch());
        return bsoncxx::types::b_date(as_time_point);
    }

}} // namespace cyberway::chaindb
