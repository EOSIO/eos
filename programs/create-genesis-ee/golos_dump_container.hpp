#pragma once

#include <cstdint>
#include <utility>
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace genesis {

struct golos_dump_header {
    char magic[13] = "";
    uint32_t version = 0;

    static constexpr auto expected_magic = "Golos\adumpOP";
    static constexpr uint32_t expected_version = 1;
};

using operation_number = std::pair<uint32_t, uint16_t>;

struct operation_header {
	operation_number num;
	uint64_t hash = 0;
};

} } // cyberway::genesis

FC_REFLECT(cyberway::genesis::operation_header, (num)(hash))
