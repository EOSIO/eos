#pragma once

#include <eosio/state_history/types.hpp>

namespace eosio {
namespace state_history {

std::vector<table_delta> create_deltas(const chain::combined_database_wrapper& db, bool full_snapshot);

} // namespace state_history
} // namespace eosio
