#include <cyberway/chaindb/names.hpp>
#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chaindb {

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
    const string names::undo_size_field  = "usz";
    const string names::undo_rec_field   = "rec";
    const string names::revision_field   = "rev";

    const string names::payer_field      = "payer";
    const string names::size_field       = "size";

    const string names::scope_path       = string(names::service_field).append(".").append(names::scope_field);
    const string names::undo_pk_path     = string(names::service_field).append(".").append(names::undo_pk_field);
    const string names::revision_path    = string(names::service_field).append(".").append(names::revision_field);

    const string names::asc_order        = "asc";
    const string names::desc_order       = "desc";
} } // namespace cyberway::chaindb
