#pragma once

#include <eosio/name.hpp>

namespace b1::rodeos {

// kv database which stores rodeos state, including a mirror of nodeos state
inline constexpr eosio::name state_database{ "eosio.state" };

// account within state_database which stores state
inline constexpr eosio::name state_account{ "state" };

} // namespace b1::rodeos
