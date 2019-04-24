#pragma once

#include <cstdint>
#include <utility>

namespace cyberway { namespace genesis {

using op_num = std::pair<int, int>;

struct golos_dump_header {
    char magic[13] = "";
    uint16_t version = 0;
    op_num op;

    static constexpr auto expected_magic = "Golos\adumpOP";
};

} } // cyberway::genesis
