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

    using std::string;

    struct mongo_asset_converter final {
        using value_type = string;

        enum value_kind {
            Unknown,
            Asset,
            Symbol
        }; // enum value_kind

        mongo_asset_converter(const bsoncxx::document::view&);

        value_kind kind() const {
            return kind_;
        }

        const value_type& value() const {
            assert(is_valid_value());
            return value_;
        }

        fc::variant get_variant() const;

        bool is_valid_value() const {
            return Unknown != kind_;
        }

    private:
        value_kind kind_ = Unknown;
        value_type value_;

        void convert(const bsoncxx::document::view&);

        void convert_to_asset(int64_t, uint64_t, const string&);
        void convert_to_symbol(uint64_t, const string&);
    }; // struct asset_converter

} } // namespace cyberway::chaindb