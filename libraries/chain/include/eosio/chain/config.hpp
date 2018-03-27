/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/asset.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <fc/time.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"

namespace eosio { namespace chain { namespace config {

typedef __uint128_t uint128_t;

const static auto default_block_log_dir     = "block_log";
const static auto default_shared_memory_dir = "shared_mem";
const static auto default_shared_memory_size = 1024*1024*1024ll;

const static uint64_t system_account_name    = N(eosio);
const static uint64_t nobody_account_name    = N(nobody);
const static uint64_t anybody_account_name   = N(anybody);
const static uint64_t producers_account_name = N(producers);
const static uint64_t eosio_auth_scope       = N(eosio.auth);
const static uint64_t eosio_all_scope        = N(eosio.all);

const static uint64_t active_name = N(active);
const static uint64_t owner_name  = N(owner);

const static int      block_interval_ms = 500;
const static int      block_interval_us = block_interval_ms*1000;
const static uint64_t block_timestamp_epoch = 946684800000ll; // epoch is year 2000.
const static uint32_t fixed_bandwidth_overhead_per_transaction = 100; // 100 bytes minimum (for signature and misc overhead)

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent_100 = 10000;
const static int percent_1   = 100;

const static uint32_t  required_producer_participation = 33 * config::percent_1;

static const uint32_t bandwidth_average_window_ms   = 24*60*60*1000l;
static const uint32_t compute_average_window_ms     = 24*60*60*1000l;
static const uint32_t blocksize_average_window_ms   = 60*1000l;


const static uint32_t   default_max_block_size              = 1024 * 1024; /// at 500ms blocks and 200byte trx, this enables 10,000 TPS burst
const static uint32_t   default_target_block_size           = default_max_block_size / 10; /// we target 1000 TPS burst
const static uint32_t   default_target_block_acts_per_scope  = 1000;
const static uint32_t   default_max_block_acts_per_scope     = default_target_block_acts_per_scope*10;

const static uint32_t   default_target_block_acts  = 2000;
const static uint32_t   default_max_block_acts     = default_target_block_acts*100;
const static uint32_t   setcode_act_usage          = 100;

const static uint64_t   default_max_storage_size       = 10 * 1024;
const static uint32_t   default_max_trx_lifetime       = 60*60;
const static uint16_t   default_max_auth_depth         = 6;
const static uint32_t   default_max_trx_runtime        = 10*1000;
const static uint16_t   default_max_inline_depth       = 4;
const static uint32_t   default_max_inline_action_size = 4 * 1024;
const static uint32_t   default_max_gen_trx_size       = 64 * 1024; ///
const static uint32_t   default_max_gen_trx_count      = 16; ///< the number of generated transactions per action
const static uint32_t   producers_authority_threshold  = 14;
const static uint32_t   rate_limiting_precision        = 1000*1000;

const static uint16_t   max_recursion_depth = 6;

const static eosio::chain::wasm_interface::vm_type default_wasm_runtime = eosio::chain::wasm_interface::vm_type::binaryen;

/**
 *  The number of sequential blocks produced by a single producer
 */
const static int producer_repetitions = 12;

/**
 * The number of blocks produced per round is based upon all producers having a chance
 * to produce all of their consecutive blocks.
 */
//const static int blocks_per_round = producer_count * producer_repetitions;

const static int irreversible_threshold_percent= 70 * percent_1;

} } } // namespace eosio::chain::config

template<typename Number>
Number EOS_PERCENT(Number value, int percentage) {
   return value * percentage / eosio::chain::config::percent_100;
}
