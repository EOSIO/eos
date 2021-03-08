#pragma once

#include <eosio/chain/types.hpp>
#include <fc/reflect/reflect.hpp>
#include <boost/container/flat_set.hpp>

namespace eosio {
namespace chain {

struct security_group_info_t {
   uint32_t                                 version = 0;
   boost::container::flat_set<account_name> participants;
};

} // namespace chain
} // namespace eosio

FC_REFLECT(eosio::chain::security_group_info_t, (version)(participants))