#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    // name can't contains _ that is why they are used for internal db and key names
        
    const string& get_system_code_name() {
        static const string name = "_CYBERWAY_";
        return name;
    }

    const string& get_unknown_name() {
        static const string name = "_UNKNOWN_";
        return name;
    }

    const string& get_undo_table_name() {
        static const string name = "UNDO";
        return name;
    }

    const string& get_code_field_name() {
        static const string name = "_CODE_";
        return name;
    }

    const string& get_table_field_name() {
        static const string name = "_TABLE_";
        return name;
    }

    const string& get_scope_field_name() {
        static const string name = "_SCOPE_";
        return name;
    }

    const string& get_payer_field_name() {
        static const string name = "_PAYER_";
        return name;
    }

    const string& get_size_field_name() {
        static const string name = "_SIZE_";
        return name;
    }

    const string& get_revision_field_name() {
        static const string name = "_REV_";
        return name;
    }

    const string& get_record_field_name() {
        static const string name = "_REC_";
        return name;
    }

    const string& get_pk_field_name() {
        static const string name = "_PK_";
        return name;
    }

} } // namespace cyberway::chaindb
