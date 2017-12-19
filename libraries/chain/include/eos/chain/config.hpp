/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/types/types.hpp>

namespace eosio { namespace config {
using types::uint16;
using types::uint32;
using types::uint64;
using types::uint128;
using types::share_type;
using types::asset;
using types::account_name;
using types::permission_name;

const static char key_prefix[] = "EOS";

const static account_name eos_contract_name = N(eos);

const static account_name nobody_account_name = N(nobody);
const static account_name anybody_account_name = N(anybody);
const static account_name producers_account_name = N(producers);

const static permission_name active_name = N(active);
const static permission_name owner_name = N(owner);

const static share_type initial_token_supply = asset::from_string("1000000000.0000 EOS").amount;

const static uint32_t default_block_interval_seconds = 1;

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent100 = 10000;
const static int percent1 = 100;

const static int default_per_auth_account_rate_time_frame_seconds = 18;
const static int default_per_auth_account_rate = 1800;
const static int default_per_code_account_rate_time_frame_seconds = 18;
const static int default_per_code_account_rate = 18000;
const static int default_per_code_account_max_db_limit_mbytes = 5;
const static int default_row_overhead_db_limit_bytes = 8 + 8 + 8 + 8; // storage for scope/code/table + 8 extra

const static int default_pending_txn_depth_limit = 1000;
const static fc::microseconds default_gen_block_time_limit = fc::milliseconds(200);

const static uint32 required_producer_participation = 33 * config::percent1;

const static uint32 default_max_block_size = 5 * 1024 * 1024;
const static uint32 default_target_block_size = 128 * 1024;
const static uint64 default_max_storage_size = 10 * 1024;
const static share_type default_elected_pay = asset(100).amount;
const static share_type default_runner_up_pay = asset(75).amount;
const static share_type default_min_eos_balance = asset(100).amount;
const static uint32 default_max_trx_lifetime = 60*60;
const static uint16 default_auth_depth_limit = 6;
const static uint32 default_max_trx_runtime = 10*1000;
const static uint16 default_inline_depth_limit = 4;
const static uint32 default_max_inline_msg_size = 4 * 1024;
const static uint32 default_max_gen_trx_size = 64 * 1024;
const static uint32 producers_authority_threshold = 14;

const static int blocks_per_round = 21;
const static int voted_producers_per_round = 20;
const static int irreversible_threshold_percent = 70 * percent1;
const static int max_producer_votes = 30;

const static uint128 producer_race_lap_length = std::numeric_limits<uint128>::max();

const static auto staked_balance_cooldown_seconds = fc::days(3).to_seconds();
} } // namespace eosio::config

template<typename Number>
Number eos_percent(Number value, int percentage) {
   return value * percentage / eosio::config::percent100;
}
