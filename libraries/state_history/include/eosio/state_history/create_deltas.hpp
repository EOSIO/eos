#pragma once

#include <eosio/state_history/types.hpp>
#include <b1/chain_kv/chain_kv.hpp>

namespace eosio {
namespace state_history {

std::vector<table_delta> create_deltas(const chainbase::database& db, bool full_snapshot);
std::vector<table_delta> create_deltas(const b1::chain_kv::database& db, bool full_snapshot);

} // namespace state_history
} // namespace eosio
