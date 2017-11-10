/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/name.hpp>
#include <fc/time.hpp>

namespace eosio { namespace config {

typedef __uint128_t uint128_t;

const static int producer_count = 21;

const static uint64_t system_account_name    = N(eosio);
const static uint64_t nobody_account_name    = N(nobody);
const static uint64_t anybody_account_name   = N(anybody);
const static uint64_t producers_account_name = N(producers);

const static uint64_t active_name = N(active);
const static uint64_t owner_name  = N(owner);

const static uint64_t initial_token_supply = 900*1000*1000*10000ll;  /// 900,000,000.0000

const static int      block_interval_ms = 500;
const static uint64_t block_timestamp_epoch = 946684800000ll; // epoch is year 2000.

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent_100 = 10000;
const static int percent_1   = 100;

const static int DefaultPerAuthAccountTimeFrameSeconds = 18;
const static int DefaultPerAuthAccount = 1800;
const static int DefaultPerCodeAccountTimeFrameSeconds = 18;
const static int DefaultPerCodeAccount = 18000;

const static uint32_t DefaultMaxBlockSize = 5 * 1024 * 1024;
const static uint32_t DefaultTargetBlockSize = 128 * 1024;
const static uint64_t DefaultMaxStorageSize = 10 * 1024;
const static uint32_t DefaultMaxTrxLifetime = 60*60;
const static uint16_t DefaultAuthDepthLimit = 6;
const static uint32_t DefaultMaxTrxRuntime = 10*1000;
const static uint16_t DefaultInlineDepthLimit = 4;
const static uint32_t DefaultMaxInlineMsgSize = 4 * 1024;
const static uint32_t DefaultMaxGenTrxSize = 64 * 1024;
const static uint32_t ProducersAuthorityThreshold = 14;

/**
 *  The number of sequential blocks produced by a single producer
 */
const static int producer_repititions = 4;

/**
 * The number of blocks produced per round is based upon all producers having a chance
 * to produce all of their consecutive blocks.
 */
const static int blocks_per_round = producer_count * producer_repititions;

const static int irreversible_threshold_percent= 70 * percent_1;
const static int max_producer_votes = 30;

const static auto staked_balance_cooldown_sec  = fc::days(3).to_seconds();
} } // namespace eosio::config

template<typename Number>
Number EOS_PERCENT(Number value, int percentage) {
   return value * percentage / eosio::config::percent_100;
}
