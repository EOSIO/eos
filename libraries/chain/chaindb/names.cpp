#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    // name can't contains _ that is why they are used for internal db and key names

    const string names::unknown         = "_UNKNOWN_";

    const string names::system_code     = "_CYBERWAY_";

    const string names::undo_table      = "undo";

    const string names::code_field      = "_CODE_";
    const string names::table_field     = "_TABLE_";
    const string names::scope_field     = "_SCOPE_";
    const string names::pk_field        = "_PK_";

    const string names::undo_pk_field   = "_UPK_";
    const string names::revision_field  = "_REV";
    const string names::operation_field = "_OP_";

    const string names::payer_field     = "_PAYER_";
    const string names::size_field      = "_SIZE_";

    const string names::asc_order       = "asc";
    const string names::desc_order      = "desc";

} } // namespace cyberway::chaindb
