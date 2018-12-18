#pragma once
#include <string>

namespace eosio { namespace chain {

using domain_name = std::string;

void validate_domain_name(const domain_name& n);

} } // eosio::chain

