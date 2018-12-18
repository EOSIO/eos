#include <climits>

#include <bsoncxx/builder/basic/document.hpp>

#include <fc/variant_object.hpp>
#include <fc/crypto/hex.hpp>

#include <cyberway/chaindb/mongo_driver_utils.hpp>
#include <cyberway/chaindb/mongo_big_int_converter.hpp>

namespace cyberway { namespace chaindb {

    namespace {
        constexpr size_t NUMBER_128_BITS        = sizeof(__int128) * CHAR_BIT;
        constexpr size_t NUMBER_128_BLOB_SIZE   = sizeof(__int128) + 1;
    }

    const std::string mongo_big_int_converter::BINARY_FIELD = "binary";
    const std::string mongo_big_int_converter::STRING_FIELD = "string";

    mongo_big_int_converter::mongo_big_int_converter(const bsoncxx::document::view& document) {
        auto binary_it = document.find(BINARY_FIELD);
        auto string_it = document.find(STRING_FIELD);

        if (binary_it == document.end() || string_it == document.end()) {
            type_ = type::invalid;
            return;
        }

        parse_binary(cyberway::chaindb::build_blob_content(binary_it->get_binary()));
    }

    void mongo_big_int_converter::parse_binary(const std::vector<char> &bytes) {

        if (bytes.size() != NUMBER_128_BLOB_SIZE) {
            type_ = type::invalid;
            return;
        }
        auto it = bytes.begin();

        __int128 val = 0;

        const char type = *it;

        for (++it; it != bytes.end(); ++it) {
            val <<= CHAR_BIT;
            val |= (0xFF & *it);
        }

        if (type == 0) {
            type_ = type::int128;
            value_.int128_val = val;
        } else {
            type_ = type::uint128;
            value_.uint128_val = static_cast<unsigned __int128>(val);
        }
    }

    mongo_big_int_converter::mongo_big_int_converter(const __int128 &int128_val) :
        type_(type::int128) {
        value_.int128_val = int128_val;
    }

    mongo_big_int_converter::mongo_big_int_converter(const unsigned __int128 &uint128_val) :
        type_(type::uint128) {
        value_.uint128_val = uint128_val;
    }

    fc::variant mongo_big_int_converter::get_raw_value() const {
        switch (type_) {
            case type::int128: return fc::variant(value_.int128_val);
            case type::uint128: return fc::variant(value_.uint128_val);
            default: return {};
        }
    }

    bool mongo_big_int_converter::is_valid_value() const {
        return type_ != type::invalid;
    }

    fc::variant_object mongo_big_int_converter::as_object_encoded() const {
        fc::mutable_variant_object object;

        fc::string as_string;
        fc::blob blob;

        switch (type_) {
            case type::int128:
                as_string = boost::lexical_cast<std::string>(value_.int128_val);
                blob.data = get_int128_blob();
                break;
            case type::uint128:
                as_string = boost::lexical_cast<std::string>(value_.uint128_val);
                blob.data = get_uint128_blob();
                break;
            default: return {};
        }

        object.set(BINARY_FIELD, blob);
        object.set(STRING_FIELD, as_string);

        return object;
    }

    std::vector<char> mongo_big_int_converter::get_int128_blob() const {
        std::vector<char> blob(NUMBER_128_BLOB_SIZE);

        auto itr = blob.begin();
        if (value_.int128_val >=0) {
            *itr = 1;
        }

        fill_blob(++itr, static_cast<unsigned __int128>(value_.int128_val));
        return blob;
    }

    std::vector<char> mongo_big_int_converter::get_uint128_blob() const {
        std::vector<char> blob(NUMBER_128_BLOB_SIZE);
        auto itr = blob.begin();

        *itr = 1;
        fill_blob(++itr, value_.uint128_val);
        return blob;
    }

    void mongo_big_int_converter::fill_blob(std::vector<char>::iterator &it, const unsigned __int128& val) const {
        for (size_t i = CHAR_BIT; i <= NUMBER_128_BITS; i += CHAR_BIT, ++it) {
            const char byte = static_cast<char>(val >> (NUMBER_128_BITS - i));
            *it = byte;
        }
    }

}} // namespace cyberway::chaindb
