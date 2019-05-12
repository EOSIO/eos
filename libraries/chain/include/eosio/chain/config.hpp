/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/wasm_interface.hpp>
#include <fc/time.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"

namespace eosio { namespace chain {

namespace resource_limits {
enum resource_id { CPU, NET, RAM, STORAGE, resources_num };
}

namespace config {

typedef __uint128_t uint128_t;

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent_100 = 10000;
const static int percent_1   = 100;

static constexpr uint64_t _KB = 1024;
static constexpr uint64_t _MB = _KB * 1024;
static constexpr uint64_t _GB = _MB * 1024;

const static auto default_blocks_dir_name    = "blocks";
const static auto reversible_blocks_dir_name = "reversible";
const static auto default_reversible_cache_size = 340*_MB;/// 1MB * 340 blocks based on 21 producer BFT delay
const static auto default_reversible_guard_size = 2*_MB;/// 1MB * 340 blocks based on 21 producer BFT delay

const static auto default_state_dir_name     = "state";
const static auto forkdb_filename            = "forkdb.dat";
const static auto default_state_size            = _GB;
const static auto default_state_guard_size      =    128*_MB;
const static uint64_t default_virtual_ram_limit = 8*_GB;
const static uint64_t default_reserved_ram_size = 512*_MB;
const static uint64_t min_resource_usage_pct = percent_1 / 10;

const static uint64_t system_account_name    = N(cyber);
const static uint64_t msig_account_name      = N(cyber.msig);
const static uint64_t null_account_name      = N(cyber.null);
const static uint64_t producers_account_name = N(cyber.prods);
const static uint64_t token_account_name     = N(cyber.token);
const static uint64_t domain_account_name    = N(cyber.domain);
const static uint64_t govern_account_name    = N(cyber.govern);
const static uint64_t stake_account_name     = N(cyber.stake);

// Active permission of producers account requires greater than 2/3 of the producers to authorize
const static uint64_t majority_producers_permission_name = N(prod.major); // greater than 1/2 of producers needed to authorize
const static uint64_t minority_producers_permission_name = N(prod.minor); // greater than 1/3 of producers needed to authorize0

const static uint64_t eosio_auth_scope       = N(cyber.auth);
const static uint64_t eosio_all_scope        = N(cyber.all);

const static uint64_t active_name = N(active);
const static uint64_t owner_name  = N(owner);
const static uint64_t eosio_any_name = N(cyber.any);
const static uint64_t eosio_code_name = N(cyber.code);

const static int      block_interval_ms = 3000;
const static int      block_interval_us = block_interval_ms*1000;
const static uint64_t block_timestamp_epoch = 946684800000ll; // epoch is year 2000.
static constexpr int64_t blocks_per_year = int64_t(365)*24*60*60*1000/block_interval_ms;

const static uint32_t   rate_limiting_precision        = 1000*1000;

using resource_limits_t  = std::array<uint64_t, resource_limits::resources_num>;
using resource_pct_t     = std::array<uint16_t, resource_limits::resources_num>;
using resource_windows_t = std::array<uint32_t, resource_limits::resources_num>;

static constexpr uint32_t _MINUTE = 60*1000;
static constexpr uint32_t _HOUR = 60*_MINUTE;
static constexpr uint32_t _DAY = 24*_HOUR;
static constexpr uint32_t _MONTH = 30*_DAY;

//////////////////////////////////////////////////////////////////////     CPU             NET            RAM             STORAGE                                  
static constexpr resource_limits_t  default_target_virtual_limits      = {{250'000,       512*_KB,       64*_MB,         32*_GB / blocks_per_year}};
static constexpr resource_limits_t  default_min_virtual_limits         = {{2'500'000,     1*_MB,         32*_MB,         16*_GB  / blocks_per_year}};
static constexpr resource_limits_t  default_max_virtual_limits         = {{2'500'000'000, 1000*_MB,      1000*32*_MB,    100*32*_GB / blocks_per_year}};
static constexpr resource_windows_t default_usage_windows              = {{_MINUTE,       _MINUTE,       _MINUTE,         _DAY}};
static constexpr resource_pct_t     default_virtual_limit_decrease_pct = {{100,           100,           100,            100}};
static constexpr resource_pct_t     default_virtual_limit_increase_pct = {{10,            10,            10,             10}};
                                                                
static constexpr resource_windows_t default_account_usage_windows      = {{_HOUR,         _HOUR,         _HOUR,          _MONTH}};
                                                                
static constexpr resource_limits_t  max_block_usage                    = {{2'500'000,     1*_MB,         128*_MB,         32*_MB}};
static constexpr resource_limits_t  max_transaction_usage              = {{2'000'000,     512*_KB,       64*_MB,          16*_MB}};

static constexpr uint64_t ram_load_multiplier = 2;

const static uint32_t   maximum_block_size                           = 2*_MB; // maximum block size for pack/unpack data in block_log
const static uint32_t   default_base_per_transaction_net_usage       = 12;  // 12 bytes (11 bytes for worst case of transaction_receipt_header + 1 byte for static_variant tag)
const static uint32_t   default_net_usage_leeway                     = 500; // TODO: is this reasonable?
const static uint32_t   default_context_free_discount_net_usage_num  = 20; // TODO: is this reasonable?
const static uint32_t   default_context_free_discount_net_usage_den  = 100;
const static uint32_t   transaction_id_net_usage                     = 32; // 32 bytes for the size of a transaction id

const static uint32_t   default_min_transaction_cpu_usage           = 100; /// min trx cpu usage in microseconds (10000 TPS equiv)
const static uint64_t   default_min_transaction_ram_usage           = 1*_KB;

const static uint32_t   default_max_trx_lifetime               = 60*60; // 1 hour
const static uint32_t   default_deferred_trx_expiration_window = 10*60; // 10 minutes
const static uint32_t   default_max_trx_delay                  = 45*24*3600; // 45 days
const static uint32_t   default_max_inline_action_size         = 4*_KB;
const static uint16_t   default_max_inline_action_depth        = 4;
const static uint16_t   default_max_auth_depth                 = 6;
const static uint32_t   default_sig_cpu_bill_pct               = 50 * percent_1; // billable percentage of signature recovery
const static uint16_t   default_controller_thread_pool_size    = 2;

const static uint32_t   min_net_usage_delta_between_base_and_max_for_trx  = 10*_KB;
// Should be large enough to allow recovery from badly set blockchain parameters without a hard fork
// (unless net_usage_leeway is set to 0 and so are the net limits of all accounts that can help with resetting blockchain parameters).

const static uint32_t   fixed_net_overhead_of_packed_trx = 16; // TODO: is this reasonable?

const static uint32_t   fixed_overhead_shared_vector_ram_bytes = 16; ///< overhead accounts for fixed portion of size of shared_vector field
const static uint32_t   overhead_per_row_per_index_ram_bytes = 32;    ///< overhead accounts for basic tracking structures in a row per index
const static uint32_t   overhead_per_account_ram_bytes     = 2*_KB; ///< overhead accounts for basic account storage and pre-pays features like account recovery
const static uint32_t   setcode_ram_bytes_multiplier       = 10;     ///< multiplier on contract size to account for multiple copies and cached compilation

const static uint32_t   hashing_checktime_block_size       = 10*_KB;  /// call checktime from hashing intrinsic once per this number of bytes

const static eosio::chain::wasm_interface::vm_type default_wasm_runtime = eosio::chain::wasm_interface::vm_type::wabt;
const static uint32_t   default_abi_serializer_max_time_ms = 15*1000; ///< default deadline for abi serialization methods

/**
 *  The number of sequential blocks produced by a single producer
 */
const static int producer_repetitions = 1; //TODO: remove it
const static int max_producers = 125;

const static size_t maximum_tracked_dpos_confirmations = 1024;     ///<
static_assert(maximum_tracked_dpos_confirmations >= ((max_producers * 2 / 3) + 1) * producer_repetitions, "Settings never allow for DPOS irreversibility");
static_assert(block_interval_ms > 0, "config::block_interval_ms must be positive");

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
