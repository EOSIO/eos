#pragma once

#include <eosio/name.hpp>

namespace b1::rodeos {

// account which stores nodeos state
inline constexpr eosio::name state_account{ "eosio.state" };

} // namespace b1::rodeos
