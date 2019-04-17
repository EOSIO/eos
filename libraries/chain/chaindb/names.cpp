#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::name_type_exception;

    string db_name_to_string(const uint64_t& value) {
        // Copy-paste of eosio::name::to_string(),
        //     but instead of the symbol '.' the symbol '_' is used
        static const char* charmap = "_12345abcdefghijklmnopqrstuvwxyz";

        string str(13,'_');

        uint64_t tmp = value;

        str[12] = charmap[tmp & 0x0f];
        tmp >>= 4;

        for (uint32_t i = 1; i <= 12; ++i) {
            char c = charmap[tmp & 0x1f];
            str[12-i] = c;
            tmp >>= 5;
        }

        boost::algorithm::trim_right_if( str, [](char c){ return c == '_'; } );
        return str;
    }

    uint64_t db_char_to_symbol(char c) {
        if (c >= 'a' && c <= 'z') {
            return (c - 'a') + 6;
        }
        if (c >= '1' && c <= '5') {
            return (c - '1') + 1;
        }

        CYBERWAY_ASSERT('_' == c, name_type_exception,
            "Name contains bad character '${c}'", ("c", string(1, c)));
        return 0;
    }

    uint64_t db_string_to_name(const char* str) {
        const auto len = strnlen(str, 14);
        CYBERWAY_ASSERT(len <= 13, name_type_exception,
            "Name is longer than 13 characters (${name}...)", ("name", string(str, len)));

        uint64_t value = 0;
        int i = 0;
        for ( ; str[i] && i < 12; ++i) {
            value |= (db_char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
        }

        if (len == 13) {
            value |= db_char_to_symbol(str[12]) & 0x0F;
        }
        return value;
    }

    // name can't contains _ that is why they are used for internal db and key names

    const string names::unknown          = "_UNKNOWN_";

    const string names::system_code      = "_CYBERWAY_";

    const string names::undo_table       = "undo";

    const string names::service_field    = "_SERVICE_";

    const string names::code_field       = "code";
    const string names::table_field      = "table";
    const string names::scope_field      = "scope";
    const string names::pk_field         = "pk";

    const string names::next_pk_field    = "npk";
    const string names::undo_pk_field    = "upk";
    const string names::undo_payer_field = "upr";
    const string names::undo_owner_field = "uow";
    const string names::undo_size_field  = "usz";
    const string names::undo_rec_field   = "rec";
    const string names::revision_field   = "rev";

    const string names::payer_field      = "payer";
    const string names::owner_field      = "owner";
    const string names::size_field       = "size";

    const string names::scope_path       = string(names::service_field).append(".").append(names::scope_field);
    const string names::undo_pk_path     = string(names::service_field).append(".").append(names::undo_pk_field);
    const string names::revision_path    = string(names::service_field).append(".").append(names::revision_field);

    const string names::asc_order        = "asc";
    const string names::desc_order       = "desc";
} } // namespace cyberway::chaindb
