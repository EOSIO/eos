#pragma once
#include <fc/reflect/reflect.hpp>

namespace cw {

struct state_header {
    char magic[12] = "Golos\astatE";
    uint32_t version;
};
struct table_header {
    uint32_t type_id;
    uint32_t records_count;
};

} // cw

FC_REFLECT(cw::state_header, (magic)(version))
FC_REFLECT(cw::table_header, (type_id)(records_count))
