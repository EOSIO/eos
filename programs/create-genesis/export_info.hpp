#pragma once

#include <eosio/chain/name.hpp>
#include <fc/container/flat_fwd.hpp>
#include <fc/container/flat.hpp>
#include <fc/variant.hpp>

namespace cyberway { namespace genesis {

using mvo = fc::mutable_variant_object;
using namespace eosio::chain;
using acc_idx = uint32_t;

struct export_info {
    fc::flat_map<acc_idx, mvo> account_infos;
    std::vector<mvo> currency_stats;
    std::vector<mvo> balance_events;
};

}} // cyberway::genesis
