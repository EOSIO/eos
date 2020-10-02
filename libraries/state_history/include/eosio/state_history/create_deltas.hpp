#pragma once

#include <eosio/state_history/types.hpp>
#include <eosio/chain/combined_database.hpp>

namespace eosio {
namespace state_history {

std::vector<table_delta> create_deltas(const chainbase::database& db, bool full_snapshot);
std::vector<table_delta> create_deltas(const session::undo_stack<chain::rocks_db_type>& undo_stack, bool full_snapshot);

} // namespace state_history
} // namespace eosio
