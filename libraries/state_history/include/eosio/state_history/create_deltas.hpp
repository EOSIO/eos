#pragma once

#include <eosio/chain/combined_database.hpp>
#include <eosio/state_history/types.hpp>

namespace eosio {
namespace state_history {

std::vector<table_delta> create_deltas(const chainbase::database& db, bool full_snapshot);
std::vector<table_delta> create_deltas(const chain::rocks_db_type& db, bool full_snapshot);

} // namespace state_history
} // namespace eosio
