#pragma once

#include <string>
#include <vector>

#include <bsoncxx/config/prelude.hpp>

namespace bsoncxx {
BSONCXX_INLINE_NAMESPACE_BEGIN
    namespace document {
        class view;
    } // namespace document
BSONCXX_INLINE_NAMESPACE_END
} // namespace bsoncxx

namespace fc {
    class variant;
} // namespace fc

namespace cyberway { namespace chaindb {

    class mongo_bigint_converter {
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
        static const std::string BINARY_FIELD;
        static const std::string STRING_FIELD;

    public:
        mongo_bigint_converter(const bsoncxx::document::view& document);
        mongo_bigint_converter(const __int128& int128_val);
        mongo_bigint_converter(const unsigned __int128& int128_val);

        bool is_valid_value() const;

        fc::variant get_raw_value() const;

        std::vector<char> get_blob_value() const;
        std::string get_string_value() const;

    private:
        std::vector<char> get_int128_blob() const;
        std::vector<char> get_uint128_blob() const;
        void fill_blob(std::vector<char>::iterator& it, const unsigned __int128& val) const;

        void parse_binary(const std::vector<char>& bytes);

    private:
        type type_;
        raw_value value_;
    };

} } // namespace cyberway::chaindb
