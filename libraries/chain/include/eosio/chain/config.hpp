/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/wasm_interface.hpp>
#include <fc/time.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"

namespace eosio { namespace chain { namespace config {

typedef __uint128_t uint128_t;

const static auto default_blocks_dir_name    = "blocks";
const static auto reversible_blocks_dir_name = "reversible";
const static auto default_reversible_cache_size = 340*1024*1024ll;/// 1MB * 340 blocks based on 21 producer BFT delay

const static auto default_state_dir_name     = "state";
const static auto forkdb_filename            = "forkdb.dat";
const static auto default_state_size            = 1*1024*1024*1024ll;


const static uint64_t system_account_name    = N(eosio);
const static uint64_t null_account_name      = N(eosio.null);
const static uint64_t producers_account_name = N(eosio.prods);

// Active permission of producers account requires greater than 2/3 of the producers to authorize
const static uint64_t majority_producers_permission_name = N(prod.major); // greater than 1/2 of producers needed to authorize
const static uint64_t minority_producers_permission_name = N(prod.minor); // greater than 1/3 of producers needed to authorize0

const static uint64_t eosio_auth_scope       = N(eosio.auth);
const static uint64_t eosio_all_scope        = N(eosio.all);

const static uint64_t active_name = N(active);
const static uint64_t owner_name  = N(owner);
const static uint64_t eosio_any_name = N(eosio.any);
const static uint64_t eosio_code_name = N(eosio.code);

const static int      block_interval_ms = 500;
const static int      block_interval_us = block_interval_ms*1000;
const static uint64_t block_timestamp_epoch = 946684800000ll; // epoch is year 2000.

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent_100 = 10000;
const static int percent_1   = 100;

const static uint32_t  required_producer_participation = 33 * config::percent_1;

static const uint32_t account_cpu_usage_average_window_ms  = 24*60*60*1000l;
static const uint32_t account_net_usage_average_window_ms  = 24*60*60*1000l;
static const uint32_t block_cpu_usage_average_window_ms    = 60*1000l;
static const uint32_t block_size_average_window_ms         = 60*1000l;

//const static uint64_t   default_max_storage_size       = 10 * 1024;
//const static uint32_t   default_max_trx_runtime        = 10*1000;
//const static uint32_t   default_max_gen_trx_size       = 64 * 1024;

const static uint32_t   rate_limiting_precision        = 1000*1000;


const static uint32_t   default_max_block_net_usage                 = 1024 * 1024; /// at 500ms blocks and 200byte trx, this enables ~10,000 TPS burst
const static uint32_t   default_target_block_net_usage_pct           = 10 * percent_1; /// we target 1000 TPS
const static uint32_t   default_max_transaction_net_usage            = default_max_block_net_usage / 2;
const static uint32_t   default_base_per_transaction_net_usage       = 12;  // 12 bytes (11 bytes for worst case of transaction_receipt_header + 1 byte for static_variant tag)
const static uint32_t   default_net_usage_leeway                     = 500; // TODO: is this reasonable?
const static uint32_t   default_context_free_discount_net_usage_num  = 20; // TODO: is this reasonable?
const static uint32_t   default_context_free_discount_net_usage_den  = 100;
const static uint32_t   transaction_id_net_usage                     = 32; // 32 bytes for the size of a transaction id

const static uint32_t   default_max_block_cpu_usage                 = 100'000; /// max block cpu usage in microseconds
const static uint32_t   default_target_block_cpu_usage_pct          = 5 * percent_1; /// target 1000 TPS
const static uint32_t   default_max_transaction_cpu_usage           = default_max_block_cpu_usage / 2; /// max trx cpu usage in microseconds
const static uint32_t   default_min_transaction_cpu_usage           = 100; /// min trx cpu usage in microseconds (10000 TPS equiv)

const static uint32_t   default_max_trx_lifetime               = 60*60; // 1 hour
const static uint32_t   default_deferred_trx_expiration_window = 10*60; // 10 minutes
const static uint32_t   default_max_trx_delay                  = 45*24*3600; // 45 days
const static uint32_t   default_max_inline_action_size         = 4 * 1024;   // 4 KB
const static uint16_t   default_max_inline_action_depth        = 4;
const static uint16_t   default_max_auth_depth                 = 6;
const static uint32_t   default_max_gen_trx_count              = 16;

const static uint32_t   fixed_net_overhead_of_packed_trx = 16; // TODO: is this reasonable?

const static uint32_t   fixed_overhead_shared_vector_ram_bytes = 16; ///< overhead accounts for fixed portion of size of shared_vector field
const static uint32_t   overhead_per_row_per_index_ram_bytes = 32;    ///< overhead accounts for basic tracking structures in a row per index
const static uint32_t   overhead_per_account_ram_bytes     = 2*1024; ///< overhead accounts for basic account storage and pre-pays features like account recovery
const static uint32_t   setcode_ram_bytes_multiplier       = 10;     ///< multiplier on contract size to account for multiple copies and cached compilation

const static uint32_t   hashing_checktime_block_size       = 10*1024;  /// call checktime from hashing intrinsic once per this number of bytes

const static eosio::chain::wasm_interface::vm_type default_wasm_runtime = eosio::chain::wasm_interface::vm_type::binaryen;

/**
 *  The number of sequential blocks produced by a single producer
 */
const static int producer_repetitions = 12;
const static int max_producers = 125;

const static size_t maximum_tracked_dpos_confirmations = 1024;     ///<
static_assert(maximum_tracked_dpos_confirmations >= ((max_producers * 2 / 3) + 1) * producer_repetitions, "Settings never allow for DPOS irreversibility" );


/**
 * The number of blocks produced per round is based upon all producers having a chance
 * to produce all of their consecutive blocks.
 */
//const static int blocks_per_round = producer_count * producer_repetitions;

const static int irreversible_threshold_percent= 70 * percent_1;

const static uint64_t billable_alignment = 16;

template<typename T>
struct billable_size;

template<typename T>
constexpr uint64_t billable_size_v = ((billable_size<T>::value + billable_alignment - 1) / billable_alignment) * billable_alignment;


} } } // namespace eosio::chain::config

constexpr uint64_t EOS_PERCENT(uint64_t value, uint32_t percentage) {
   return (value * percentage) / eosio::chain::config::percent_100;
}

template<typename Number>
Number EOS_PERCENT_CEIL(Number value, uint32_t percentage) {
   return ((value * percentage) + eosio::chain::config::percent_100 - eosio::chain::config::percent_1)  / eosio::chain::config::percent_100;
}
