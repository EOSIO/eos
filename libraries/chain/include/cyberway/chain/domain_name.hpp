#pragma once
#include <string>

namespace cyberway { namespace chain {

using domain_name = std::string;    // TODO: separate class to embed restrictions into deserializer
using username = std::string;

void validate_domain_name(const domain_name& n);
void validate_username(const username& n);

} } // cyberway::chain

