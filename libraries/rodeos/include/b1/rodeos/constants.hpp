#pragma once

#include <eosio/name.hpp>

namespace b1::rodeos {

// Used as db parameter to kv_table, not used for anything anymore
inline constexpr eosio::name state_database{ "eosio.state" };

// account which stores nodeos state
inline constexpr eosio::name state_account{ "eosio.state" };

} // namespace b1::rodeos
