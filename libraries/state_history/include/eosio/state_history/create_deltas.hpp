#pragma once

#include <eosio/state_history/types.hpp>
#include <eosio/chain/kv_database.hpp>

namespace eosio {
namespace state_history {

std::vector<table_delta> create_deltas(const chain::kv_database& db, bool full_snapshot);

} // namespace state_history
} // namespace eosio
