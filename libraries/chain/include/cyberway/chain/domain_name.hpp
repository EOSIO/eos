#pragma once
#include <string>

namespace cyberway { namespace chain {
// constants
constexpr size_t domain_max_size = 253;
constexpr size_t domain_min_part_size = 1;
constexpr size_t domain_max_part_size = 63;

constexpr size_t username_max_size = 32;        // it's 16 in Golos
constexpr size_t username_min_part_size = 1;    // it's 3 in Golos
constexpr size_t username_max_part_size = username_max_size;

using domain_name = std::string;    // TODO: separate class to embed restrictions into deserializer
using username = std::string;

void validate_domain_name(const domain_name& n);
void validate_username(const username& n);

} } // cyberway::chain

