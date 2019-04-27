#pragma once

#include <cstdint>
#include <utility>

namespace cyberway { namespace genesis {

using operation_number = std::pair<uint32_t, uint16_t>;

struct golos_dump_header {
    char magic[13] = "";
    uint32_t version = 0;

    static constexpr auto expected_magic = "Golos\adumpOP";
    static constexpr uint32_t expected_version = 1;
};

} } // cyberway::genesis
