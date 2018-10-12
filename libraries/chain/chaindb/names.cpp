#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    // name can't contains _ that is why they are used for internal db and key names
        
    const string& system_code_name() {
        static const string name = "_CYBERWAY_";
        return name;
    }

    const string& unknown_name() {
        static const string name = "_UNKNOWN_";
        return name;
    }

    const string& scope_field_name() {
        static const string name = "_SCOPE_";
        return name;
    }

    const string& payer_field_name() {
        static const string name = "_PAYER_";
        return name;
    }

    const string& size_field_name() {
        static const string name = "_SIZE_";
        return name;
    }

} } // namespace cyberway::chaindb
