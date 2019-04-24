#pragma once

#include <eosio/chain/asset.hpp>

#include <fc/fixed_string.hpp>
#include <fc/container/flat_fwd.hpp>
#include <fc/container/flat.hpp>

namespace cyberway { namespace golos {

using std::string;

using account_name_type = fc::fixed_string<>;

using fc::flat_set;

using eosio::chain::asset;

} } // cyberway::golos
