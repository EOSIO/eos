#pragma once

#include <string>
#include <vector>

#include <bsoncxx/config/prelude.hpp>
#include <bsoncxx/document/view.hpp>


namespace fc {
    struct variant_object;
    struct variant;
}

namespace cyberway { namespace chaindb {

    class mongo_big_int_converter {
        union raw_value {
            __int128            int128_val;
            unsigned __int128   uint128_val;
        };

        enum class type {
            int128,
            uint128,
            invalid
        };

    public:
        mongo_big_int_converter(const bsoncxx::document::view& document);
        mongo_big_int_converter(const __int128& int128_val);
        mongo_big_int_converter(const unsigned __int128& int128_val);

        bool is_valid_value() const;

        fc::variant get_raw_value() const;
        fc::variant_object as_object_encoded() const;

    private:
        std::vector<char> get_int128_blob() const;
        std::vector<char> get_uint128_blob() const;
        void fill_blob(std::vector<char>::iterator& it, const unsigned __int128& val) const;

        void parse_binary(const std::vector<char>& bytes);

    public:
        static const std::string BINARY_FIELD;
        static const std::string STRING_FIELD;

    private:
        type type_;
        raw_value value_;
    };


}} // namespace cyberway::chaindb
