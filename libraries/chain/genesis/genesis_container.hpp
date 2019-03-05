#pragma once
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace genesis {

struct state_header {
    char magic[12];
    uint32_t version;

    static constexpr auto expected_magic = "Golos\astatE";
};

struct table_header {
    uint32_t type_id;
    uint32_t records_count;
};

}} // cyberway::genesis

// FC_REFLECT(cyberway::genesis::state_header, (magic)(version))    // won't work with char[12]
FC_REFLECT(cyberway::genesis::table_header, (type_id)(records_count))
