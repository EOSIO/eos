#pragma once
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace genesis {

struct golos_state_header {
    char magic[12];
    uint32_t version;
    uint32_t block_num;

    static constexpr auto expected_magic = "Golos\astatE";
};

struct golos_table_header {
    uint32_t type_id;
    uint32_t records_count;
};

}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::golos_table_header, (type_id)(records_count))
