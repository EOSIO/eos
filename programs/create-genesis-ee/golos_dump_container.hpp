#pragma once

#include <cstdint>
#include <utility>
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace golos {

struct golos_dump_header {
    char magic[13] = "";
    uint32_t version = 0;

    static constexpr auto expected_magic = "Golos\adumpOP";
    static constexpr uint32_t expected_version = 1;
};

using operation_number = std::pair<uint32_t, uint16_t>;

struct operation {
    operation_number num;
    uint64_t offset = 0; // Do not reflect
};

struct hashed_operation : operation {
    uint64_t hash = 0;
};

} } // cyberway::golos

FC_REFLECT(cyberway::golos::operation, (num))

FC_REFLECT_DERIVED(cyberway::golos::hashed_operation, (cyberway::golos::operation),(hash))

#define REFLECT_OP(OP, FIELDS) \
	FC_REFLECT_DERIVED(OP, (cyberway::golos::operation), FIELDS)

#define REFLECT_OP_HASHED(OP, FIELDS) \
	FC_REFLECT_DERIVED(OP, (cyberway::golos::hashed_operation), FIELDS)
