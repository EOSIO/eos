#include <chrono>
#include <iomanip>

#include <bsoncxx/types.hpp>

#include "fc/time.hpp"
#include "fc/variant_object.hpp"

#include "cyberway/chaindb/mongo_driver_utils.h"

namespace cyberway { namespace chaindb {

    bsoncxx::types::b_decimal128 to_decimal128(uint64_t val) {
        const auto val_as_string = std::to_string(val);
        return bsoncxx::types::b_decimal128(val_as_string);
    }

    uint64_t from_decimal128(const bsoncxx::types::b_decimal128& val) {
        char* end;
        const uint64_t as_uint64 = std::strtoull(val.value.to_string().c_str(), &end, 10);
        return as_uint64;
    }

    fc::time_point from_date(const bsoncxx::types::b_date& date) {
        return fc::time_point(fc::milliseconds(date.value.count() * 1000));
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

}} // namespace cyberway::chaindb
