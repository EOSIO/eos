/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/asset.hpp>
#include <fc/time.hpp>

namespace eosio { namespace chain { namespace config {

typedef __uint128_t uint128_t;

const static auto default_block_log_dir     = "block_log";
const static auto default_shared_memory_dir = "shared_mem";
const static auto default_shared_memory_size = 1024*1024*1024ll;
const static int producer_count = 21;

const static uint64_t system_account_name    = N(eosio);
const static uint64_t nobody_account_name    = N(nobody);
const static uint64_t anybody_account_name   = N(anybody);
const static uint64_t producers_account_name = N(producers);
const static uint64_t eosio_auth_scope       = N(eosio.auth);
const static uint64_t eosio_all_scope        = N(eosio.all);

const static uint64_t active_name = N(active);
const static uint64_t owner_name  = N(owner);

const static share_type initial_token_supply = asset::from_string("1000000000.0000 EOS").amount;

const static int      block_interval_ms = 500;
const static int      block_interval_us = block_interval_ms*1000;
const static uint64_t block_timestamp_epoch = 946684800000ll; // epoch is year 2000.

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent_100 = 10000;
const static int percent_1   = 100;

const static uint32_t   required_producer_participation = 33 * config::percent_1;

const static uint32_t   default_target_block_size      = 128 * 1024;
const static uint32_t   default_max_block_size         = 5 * 1024 * 1024;
const static uint64_t   default_max_storage_size       = 10 * 1024;
const static uint32_t   default_max_trx_lifetime       = 60*60;
const static uint16_t   default_max_auth_depth         = 6;
const static uint32_t   default_max_trx_runtime        = 10*1000;
const static uint16_t   default_max_inline_depth       = 4;
const static uint32_t   default_max_inline_action_size = 4 * 1024;
const static uint32_t   default_max_gen_trx_size       = 64 * 1024; /// 
const static uint32_t   default_max_gen_trx_count      = 16; ///< the number of generated transactions per action
const static uint32_t   producers_authority_threshold  = 14;

const static share_type default_elected_pay            = asset(100).amount;
const static share_type default_min_eos_balance        = asset(100).amount;

const static uint16_t   max_recursion_depth = 6;

/**
 *  The number of sequential blocks produced by a single producer
 */
const static int producer_repititions = 6;

/**
 * The number of blocks produced per round is based upon all producers having a chance
 * to produce all of their consecutive blocks.
 */
const static int blocks_per_round = producer_count * producer_repititions;

const static int irreversible_threshold_percent= 70 * percent_1;
const static int max_producer_votes = 30;

const static auto staked_balance_cooldown_sec  = fc::days(3).to_seconds();
} } } // namespace eosio::chain::config

template<typename Number>
Number EOS_PERCENT(Number value, int percentage) {
   return value * percentage / eosio::chain::config::percent_100;
}
